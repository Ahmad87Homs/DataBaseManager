#include "disk_manager.hpp"

#include <memory>
#include <chrono>

#include "BufferPoolManager/buffer_bool_manager.hpp"
// ---------- Example usage ----------
int main() {
  std::cout << "Main Page Size [Bytes]" <<  sizeof(DiskManager::HeapPage::data) 
            << std::endl;





BufferPoolManager::BufferPool buffer_pool(10);
auto time_start = std::chrono::high_resolution_clock::now();
{
  auto fetched_page0 = buffer_pool.fetchPage("table1", 0);
  auto fetched_page1 = buffer_pool.fetchPage("table1", 1);
  auto fetched_page2 = buffer_pool.fetchPage("table1", 2);
  auto fetched_page3 = buffer_pool.fetchPage("table1", 3);
}
auto time_end = std::chrono::high_resolution_clock::now();
auto duration = std::chrono::duration_cast<std::chrono::microseconds>(time_end - time_start).count();
std::cout << "Time taken to fetch 4 pages: " << duration << " microseconds" << std::endl;


auto time_start1 = std::chrono::high_resolution_clock::now();
{
  auto fetched_page0 = buffer_pool.fetchPage("table1", 0);
  auto fetched_page1 = buffer_pool.fetchPage("table1", 1);
  auto fetched_page2 = buffer_pool.fetchPage("table1", 2);
  auto fetched_page3 = buffer_pool.fetchPage("table1", 3);
}
auto time_end1 = std::chrono::high_resolution_clock::now();
auto duration1 = std::chrono::duration_cast<std::chrono::microseconds>(time_end1 - time_start1).count();
std::cout << "Time taken to fetch 4 pages: " << duration1 << " microseconds" << std::endl;

buffer_pool.writePage("table1", 0);
buffer_pool.writePage("table1", 2);
buffer_pool.writePage("table1", 3);

buffer_pool.writePage("table2", 1);
auto fetched_page1 = buffer_pool.fetchPage("table1", 0);
if (fetched_page1) {
    std::cout << "Fetched Page ID from table1: " << std::endl;
} else {    
    std::cout << "Failed to fetch page 0 from table1" << std::endl;   
}

(*fetched_page1)->page->fromBytes(reinterpret_cast<const uint8_t*>("Hello, World!"));
(*fetched_page1)->is_dirty=true;
buffer_pool.writePage("table1", 0); // Mark as dirty and write back to disk

auto fetched_page2 = buffer_pool.fetchPage("table1", 2);
(*fetched_page2)->page->fromBytes(reinterpret_cast<const uint8_t*>("XXXXXXXXXXXXXXXXXXXXXXXXXXXXX"));
(*fetched_page2)->is_dirty=true;
buffer_pool.writePage("table1", 2); // Mark as dirty and write back to disk

auto fetched_page3 = buffer_pool.fetchPage("table1", 3);
(*fetched_page3)->page->fromBytes(reinterpret_cast<const uint8_t*>("YYYYYYYYYYYYYYYYYYYYYYYYYYYYYYY"));
(*fetched_page3)->is_dirty=true;
buffer_pool.writePage("table1", 3); // Mark as dirty and write back to disk
  return 0;
}
