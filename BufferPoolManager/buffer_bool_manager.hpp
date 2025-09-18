#pragma once
#include "../DiskManager/disk_manager.hpp"
#include <iostream>
#include <map>
#include <array>
#include <stdint.h>
#include <memory>
#include <optional>
#include <vector>


namespace BufferPoolManager {

constexpr size_t kTableNum=3;
constexpr std::array<const char*,kTableNum> file_list{"table1","table2","table3"};

/* [file id ] [diskhandler, [page id, [is dirty page , content ] ]    */

/**
 * @brief Represents the content of a page in the buffer pool.
 * 
 * @param is_dirty Indicates whether the page has been modified (dirty).
 * @param page Shared pointer to the actual page data.
 */
struct page_content {
    bool is_dirty; ///< Indicates if the page is dirty (modified).
    std::shared_ptr<DiskManager::IPage> page; ///< Pointer to the page data.
}; 
struct FileHandle {
    std::shared_ptr<DiskManager::DiskManager> disk_manager;
    std::map<uint32_t, page_content> page_table;
};


class BufferPool {
public:
  BufferPool(size_t pool_size);

  void writePage(const std::string& table,uint32_t page_id) ;
/* to fetch page  : table name and page id is required , and return is shared pointer to page content */
  std::optional<page_content*> fetchPage(const std::string& table, uint32_t page_id);


  ~BufferPool() ;

private:
  std::size_t buffer_size_{0};
  std::map<std::string,FileHandle> file_table;
};

} // namespace BufferPoolManager
// ---------- Buffer pool structures ----------