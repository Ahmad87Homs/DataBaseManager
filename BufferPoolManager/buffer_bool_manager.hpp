#pragma once
#include "../DiskManager/disk_manager.hpp"
#include <iostream>
#include <map>
#include <stdint.h>
namespace BufferPoolManager {

class BufferPool {
public:
  BufferPool(size_t pool_size)
      : buffer_size_(pool_size), disk_manager_(file_name_) {
    std::cout << "Buffer Pool of size " << buffer_size_ << " created.\n";
  }

  std::optional<DiskManager::Page> fetchPage(uint32_t page_id) {
    auto it = page_table_.find(page_id);
    if (it != page_table_.end()) {
      return it->second;
    }

    if (disk_manager_.readPage(page_id, page_table_[page_id])) {
      return page_table_[page_id];
    }
    return std::nullopt;
  }

  void writePage(uint32_t page_id, const DiskManager::Page &page) {
    if (page_table_.find(page_id) != page_table_.end()) {
      std::cout << "Writing page " << page_id << " to buffer pool.\n";
    } else {

      std::cerr << "Page " << page_id << " not found in buffer pool.\n";
    }
    page_table_[page_id] = page;
    disk_manager_.writePage(page_id, page);
  }

  ~BufferPool() {}

private:
  std::size_t buffer_size_{0};
  std::string file_name_{"db_pages.bin"};
  DiskManager::DiskManager disk_manager_;
  std::map<uint32_t, DiskManager::Page> page_table_;
};

} // namespace BufferPoolManager
// ---------- Buffer pool structures ----------