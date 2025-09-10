#include "disk_manager.hpp"
// ---------- Buffer pool structures ----------
struct BufferFrame {
    Page page;
    int pin_count;
    bool is_dirty;
    // constructor
    BufferFrame() : pin_count(0), is_dirty(false) { std::memset(&page, 0, sizeof(Page)); }
};

// ---------- Buffer pool ----------
class BufferPool {
public:
    BufferPool(size_t pool_size, DiskManager &disk) : pool_size_(pool_size), disk_(disk) {
        frames_.resize(pool_size_);
    }

    ~BufferPool() {
        flushAll();
    }

    // Fetch a page and pin it. If not present, load from disk (or initialize).
    // Returns pointer to Page inside a frame. Caller must call unpinPage when done.
    Page* fetchPage(uint32_t page_id) {
        // check if page in memory
        auto it = page_table_.find(page_id);
        if (it != page_table_.end()) {
            size_t frame_idx = it->second;
            frames_[frame_idx].pin_count++;
            touchLRU(frame_idx);
            return &frames_[frame_idx].page;
        }

        // need to bring page into buffer
        size_t victim = findVictim();
        if (victim == SIZE_MAX) {
            std::cerr << "BufferPool: no free frame and no victim (all pinned)\n";
            return nullptr;
        }

        // if victim dirty, flush it
        if (frames_[victim].is_dirty) {
            disk_.writePage(frames_[victim].page.page_id, frames_[victim].page);
            frames_[victim].is_dirty = false;
        }

        // remove old mapping if any
        if (frames_[victim].page.page_id != 0 || frames_[victim].pin_count != 0 || frames_[victim].is_dirty) {
            // erase only if that frame had a valid page cached
            page_table_.erase(frames_[victim].page.page_id);
        }

        // attempt to read page from disk; if not present, init a new page
        Page pg;
        if (!disk_.readPage(page_id, pg)) {
            // initialize new page
            pg.init(page_id, PageType::HEAP);
            // make sure page exists on disk for future read/write
            disk_.ensurePageExists(page_id);
        }

        // put into frame
        frames_[victim].page = pg;
        frames_[victim].pin_count = 1; // pinned by caller
        frames_[victim].is_dirty = false;
        page_table_[page_id] = victim;
        // mark as most recently used
        touchLRU(victim);
        return &frames_[victim].page;
    }

    // Unpin page and optionally mark dirty
    bool unpinPage(uint32_t page_id, bool is_dirty) {
        auto it = page_table_.find(page_id);
        if (it == page_table_.end()) return false;
        size_t frame_idx = it->second;
        if (frames_[frame_idx].pin_count <= 0) return false;
        frames_[frame_idx].pin_count--;
        if (is_dirty) frames_[frame_idx].is_dirty = true;
        if (frames_[frame_idx].pin_count == 0) {
            // eligible for replacement — ensure it is in LRU list (touch already does)
            touchLRU(frame_idx);
        }
        return true;
    }

    // flush single page to disk if present
    bool flushPage(uint32_t page_id) {
        auto it = page_table_.find(page_id);
        if (it == page_table_.end()) return false;
        size_t frame_idx = it->second;
        if (frames_[frame_idx].is_dirty) {
            disk_.writePage(page_id, frames_[frame_idx].page);
            frames_[frame_idx].is_dirty = false;
        }
        return true;
    }

    // flush all dirty frames to disk
    void flushAll() {
        for (size_t i = 0; i < pool_size_; ++i) {
            if (frames_[i].is_dirty) {
                disk_.writePage(frames_[i].page.page_id, frames_[i].page);
                frames_[i].is_dirty = false;
            }
        }
    }

private:
    // LRU helpers:
    // we keep a list of frame indices with least-recently-used at front
    // and MRU at back. Only unpinned frames are stored in this list.
    void touchLRU(size_t frame_idx) {
        // remove if already in list
        if (pos_in_lru_.count(frame_idx)) {
            auto it = pos_in_lru_[frame_idx];
            lru_list_.erase(it);
            pos_in_lru_.erase(frame_idx);
        }
        // only add if unpinned (pinned frames are not eligible for eviction)
        if (frames_[frame_idx].pin_count == 0) {
            lru_list_.push_back(frame_idx);
            auto it = lru_list_.end(); --it;
            pos_in_lru_[frame_idx] = it;
        } else {
            // ensure it's not in LRU if pinned
            pos_in_lru_.erase(frame_idx);
        }
    }

    // find a victim frame (unpin & LRU)
    size_t findVictim() {
        // first try to find an entirely empty frame (no mapping)
        for (size_t i = 0; i < pool_size_; ++i) {
            if (page_table_reverse_.count(i) == 0 && frames_[i].pin_count == 0 && frames_[i].page.page_id == 0) {
                // empty frame
                return i;
            }
        }

        // fallback to LRU list front
        if (!lru_list_.empty()) {
            size_t victim = lru_list_.front();
            lru_list_.pop_front();
            pos_in_lru_.erase(victim);
            // erase old page mapping
            page_table_.erase(frames_[victim].page.page_id);
            return victim;
        }

        // no unpinned frame available
        // try to find any frame with pin_count==0
        for (size_t i = 0; i < pool_size_; ++i) {
            if (frames_[i].pin_count == 0) {
                page_table_.erase(frames_[i].page.page_id);
                pos_in_lru_.erase(i);
                return i;
            }
        }

        return SIZE_MAX;
    }

    size_t pool_size_;
    DiskManager &disk_;
    std::vector<BufferFrame> frames_;

    // page_id -> frame index
    std::unordered_map<uint32_t, size_t> page_table_;

    // reverse map (frame -> page) - maintained implicitly by frames_.page.page_id,
    // but having an existence map for empty frames can help (optional)
    std::unordered_map<size_t, uint32_t> page_table_reverse_;

    // LRU structures (stores unpinned frames only)
    std::list<size_t> lru_list_;
    std::unordered_map<size_t, std::list<size_t>::iterator> pos_in_lru_;
};

// ---------- Simple PageManager that writes records into Page.data ----------
struct SimpleRow {
    int32_t id;
    char name[20];
};

class SimplePageManager {
public:
    // insert row into page; returns slot index or -1 on failure
    static int insert(Page &page, const SimpleRow &r) {
        // check capacity
        if (page.row_count >= kMaxSlots) return -1;
        size_t freePtr = page.free_space_ptr;
        if (freePtr + sizeof(SimpleRow) > sizeof(page.data)) return -1;

        // copy bytes
        std::memcpy(page.data + freePtr, &r, sizeof(SimpleRow));
        // record slot (offset relative to data[] start)
        page.slots[page.row_count].offset = static_cast<uint16_t>(freePtr);
        page.slots[page.row_count].length = static_cast<uint16_t>(sizeof(SimpleRow));
        page.row_count++;
        page.free_space_ptr = static_cast<uint16_t>(freePtr + sizeof(SimpleRow));
        return static_cast<int>(page.row_count - 1);
    }

    static std::optional<SimpleRow> read(const Page &page, int slot) {
        if (slot < 0 || slot >= page.row_count) return std::nullopt;
        const Slot &s = page.slots[slot];
        SimpleRow r{};
        std::memcpy(&r, page.data + s.offset, s.length);
        return r;
    }
};

// ---------- Example usage ----------
int main() {
    std::cout << "Main Page Size  " << sizeof (Page) << std::endl;
    DiskManager disk("db_pages.bin");
    BufferPool pool(3, disk); // pool with 3 frames

    // create a page and write via buffer pool
    Page *p0 = pool.fetchPage(0);
    if (!p0) { std::cerr << "Failed to fetch page 0\n"; return 1; }

    // If this page is newly created it may have page_id==0 but uninitialized; ensure init
    if (p0->page_id != 0 || p0->row_count == 0) {
        // if we loaded existing, okay; otherwise re-init
        p0->init(0);
    }

    // Insert some rows
    SimpleRow r1{1, "Alice"};
    SimpleRow r2{2, "Bob"};
    SimpleRow r3{3, "Carol"};
    SimplePageManager::insert(*p0, r1);
    SimplePageManager::insert(*p0, r2);
    SimplePageManager::insert(*p0, r3);

    // mark page dirty and unpin
    pool.unpinPage(0, true);

    // fetch same page again (should hit buffer), read rows
    Page *p0_again = pool.fetchPage(0);
    if (p0_again) {
        std::cout << "Page " << p0_again->page_id << " row_count=" << p0_again->row_count << "\n";
        for (int i = 0; i < p0_again->row_count; ++i) {
            auto r = SimplePageManager::read(*p0_again, i);
            if (r) {
                std::cout << "slot " << i << " id=" << r->id << " name=" << r->name << "\n";
            }
        }
        pool.unpinPage(0, false);
    }

    // flush all to disk
    pool.flushAll();

    // simulate program restart: create new buffer pool and read page 0 from disk
    {
        BufferPool pool2(2, disk);
        Page *p = pool2.fetchPage(0);
        if (!p) { std::cerr << "Failed to fetch page 0 after reopen\n"; return 1; }
        std::cout << "After reopen: page_id=" << p->page_id << " row_count=" << p->row_count << "\n";
        for (int i = 0; i < p->row_count; ++i) {
            auto r = SimplePageManager::read(*p, i);
            if (r) std::cout << "slot " << i << " id=" << r->id << " name=" << r->name << "\n";
        }
        pool2.unpinPage(0, false);
    }

    return 0;
}
