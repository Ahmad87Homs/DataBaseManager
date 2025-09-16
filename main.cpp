#include "disk_manager.hpp"

// ---------- Simple PageManager that writes records into Page.data ----------
struct SimpleRow {
  int32_t id;
  char name[20];
};

class SimplePageManager {
public:
  // insert row into page; returns slot index or -1 on failure
  static int insert(DiskManager::Page &page, const SimpleRow &r) {
    // check capacity
    if (page.row_count >= DiskManager::kMaxSlots)
      return -1;
    size_t freePtr = page.free_space_ptr;
    if (freePtr + sizeof(SimpleRow) > sizeof(page.data))
      return -1;

    // copy bytes
    std::memcpy(page.data + freePtr, &r, sizeof(SimpleRow));
    // record slot (offset relative to data[] start)
    page.slots[page.row_count].offset = static_cast<uint16_t>(freePtr);
    page.slots[page.row_count].length =
        static_cast<uint16_t>(sizeof(SimpleRow));
    page.row_count++;
    page.free_space_ptr = static_cast<uint16_t>(freePtr + sizeof(SimpleRow));
    return static_cast<int>(page.row_count - 1);
  }

  static std::optional<SimpleRow> read(const DiskManager::Page &page,
                                       int slot) {
    if (slot < 0 || slot >= page.row_count)
      return std::nullopt;
    const DiskManager::Slot &s = page.slots[slot];
    SimpleRow r{};
    std::memcpy(&r, page.data + s.offset, s.length);
    return r;
  }
};

#include "BufferPoolManager/buffer_bool_manager.hpp"
// ---------- Example usage ----------
int main() {
  std::cout << "Main Page Size [Bytes]" << sizeof(DiskManager::Page)
            << std::endl;
  BufferPoolManager::BufferPool pool(10); // pool with 3 frames

  // create a page and write via buffer pool
  DiskManager::Page p0 = pool.fetchPage(0).value_or(DiskManager::Page{});
  // Insert some rows
  SimpleRow r1{1, "Alice"};
  SimpleRow r2{2, "Bob"};
  SimpleRow r3{3, "Carol"};
  SimplePageManager::insert(p0, r1);
  SimplePageManager::insert(p0, r2);
  SimplePageManager::insert(p0, r3);
  std::cout << "Inserted 3 rows into page 0, row_count=" << p0.row_count
            << "\n";
  pool.writePage(0, p0);

  // read back via buffer pool
  DiskManager::Page p0_read = pool.fetchPage(0).value_or(DiskManager::Page{});
  std::cout << "Read back page 0, row_count=" << p0_read.row_count << "\n";
  for (int i = 0; i < p0_read.row_count; i++) {
    auto r = SimplePageManager::read(p0_read, i);
    if (r) {
      std::cout << " Row " << i << ": id=" << r->id << ", name=" << r->name
                << "\n";
    }
  }

  return 0;
}
