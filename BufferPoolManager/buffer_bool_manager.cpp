#include "buffer_bool_manager.hpp"

namespace BufferPoolManager {


  BufferPool::BufferPool(size_t pool_size)
      : buffer_size_(pool_size){
        for(const auto& file : file_list) {
            file_table[file] = FileHandle{std::make_shared<DiskManager::DiskManager>(std::string(file) + ".db"), {}};
        }
       std::cout << "Buffer Pool of size " << buffer_size_ << " created.\n";
  }

  void BufferPool::writePage(const std::string& table,uint32_t page_id) {
    /* to write to pgage , check if table is exsist then write page if it is exsist and dirty flag is set */
    auto table_it = file_table.find(table);
    if (table_it != file_table.end()) {
        auto& file_handle = table_it->second;
        auto page_it = file_handle.page_table.find(page_id);
        if (page_it != file_handle.page_table.end()) {
            auto& page_content = page_it->second;
            if (page_content.is_dirty) {
                file_handle.disk_manager->writePage(page_id, *page_content.page);
                page_content.is_dirty = false; // Reset dirty flag after writing
                std::cout << "Page " << page_id << " written to disk for table "<< table << ".\n";          
            } else {
                std::cout << "Page " << page_id << " is not dirty. No need to write to disk.\n";
            }
        } else {
            std::cerr << "Page " << page_id << " not found in buffer for table " << table << ".\n";
            /* create a page  */
            if (file_handle.page_table.size() >= buffer_size_) {
                std::cerr << "Buffer pool is full. Cannot add new page.\n";
                return; // Buffer pool is full
            }
            auto new_page = std::make_shared<DiskManager::HeapPage>();
            new_page->init(page_id);
            file_handle.page_table[page_id] = page_content{true, new_page};
            std::cout << "New page " << page_id << " created and added to buffer for table " << table << ".\n"; 
            /*  STORE TO DISK */
            file_handle.disk_manager->writePage(page_id, *new_page);
            file_handle.page_table[page_id].is_dirty = false; // Reset dirty flag after writing
            std::cout << "Page " << page_id << " written to disk for table "<< table << ".\n";
        }
    } else {
        std::cerr << "Table " << table << " not found.\n";      
    }
  } 
/* to fetch page  : table name and page id is required , and return is shared pointer to page content */
  std::optional<page_content*> BufferPool::fetchPage(const std::string& table, uint32_t page_id) {
    auto table_it = file_table.find(table);
    if (table_it == file_table.end()) {
        std::cerr << "Table " << table << " not found.\n";
        return std::nullopt;
    }

    auto& file_handle = table_it->second;
    auto page_it = file_handle.page_table.find(page_id);
    if (page_it != file_handle.page_table.end()) {
        // Page is already in buffer pool
        std::cout << "Page " << page_id << " found in buffer for table " << table << ".\n";
        return &page_it->second;
    }

    // Page not in buffer pool, need to load from disk
    if (file_handle.page_table.size() >= buffer_size_) {
        std::cerr << "Buffer pool is full. Cannot fetch new page.\n";
        return std::nullopt; // Buffer pool is full
    }

    auto new_page = std::make_shared<DiskManager::HeapPage>();
    if (!file_handle.disk_manager->readPage(page_id, *new_page)) {
        std::cerr << "Failed to read page " << page_id << " from disk for table " << table << ".\n";
        return std::nullopt; // Failed to read page from disk
    }

    // Add the new page to the buffer pool
    auto [it, success] = file_handle.page_table.insert({page_id, page_content{false, new_page}});
    // std::cout << "Page " << page_id << " loaded into buffer for table " << table << ".\n";
    return &it->second;
  }


  BufferPool::~BufferPool() { 
    /*STOR DIRTY PAGES */
    for (auto& [table, file_handle] : file_table) {
        for (auto& [page_id, page_content] : file_handle.page_table) {
            if (page_content.is_dirty) {
                file_handle.disk_manager->writePage(page_id, *page_content.page);
                std::cout << "Dirty page " << page_id << " written to disk for table " << table << " during buffer pool destruction.\n";
            }
        }
    }
  }





} // namespace BufferPoolManager
                               // ---------- Buffer pool structures ----------