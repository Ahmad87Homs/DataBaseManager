#include "disk_manager.hpp"
#include "page.hpp"

namespace DiskManager {

// ---------- Disk manager ----------
DiskManager::DiskManager(const std::string &filename) : file_name(filename) {
  // open file for read/write, create if not exists
  fstream_.open(file_name, std::ios::in | std::ios::out | std::ios::binary);
  if (!fstream_.is_open()) {
    // create new file
    std::ofstream create(file_name, std::ios::binary);
    create.close();
    // reopen for read/write
    fstream_.open(file_name, std::ios::in | std::ios::out | std::ios::binary);
  }
}

DiskManager::~DiskManager() {
  if (fstream_.is_open())
    fstream_.close();
}

bool DiskManager::readPage(uint32_t page_id, Page &page_out) {
  std::streampos offset = static_cast<std::streampos>(page_id) *
                          static_cast<std::streampos>(sizeof(Page));
  fstream_.seekg(0, std::ios::end);
  std::streampos fileSize = fstream_.tellg();
  if (fileSize < offset + static_cast<std::streampos>(sizeof(Page))) {
    // page does not exist on disk
    return false;
  }
  fstream_.seekg(offset, std::ios::beg);
  fstream_.read(reinterpret_cast<char *>(&page_out), sizeof(Page));
  return fstream_.gcount() == static_cast<std::streamsize>(sizeof(Page));
}

// write (overwrite) page
void DiskManager::writePage(uint32_t page_id, const Page &page) {
  std::cout << "Writing page " << page_id << " to disk at offset "
            << (static_cast<std::streampos>(page_id) *
                static_cast<std::streampos>(sizeof(Page)))
            << std::endl;
  std::streampos offset = static_cast<std::streampos>(page_id) *
                          static_cast<std::streampos>(sizeof(Page));
  fstream_.seekp(offset, std::ios::beg);
  fstream_.write(reinterpret_cast<const char *>(&page), sizeof(Page));
  fstream_.flush();
}

// ensure page exists (zero-initialize if absent)
void DiskManager::ensurePageExists(uint32_t page_id) {
  std::streampos offset = static_cast<std::streampos>(page_id) *
                          static_cast<std::streampos>(sizeof(Page));
  fstream_.seekp(0, std::ios::end);
  std::streampos fileSize = fstream_.tellp();
  if (fileSize < offset + static_cast<std::streampos>(sizeof(Page))) {
    // extend file with zeros
    fstream_.seekp(offset + static_cast<std::streampos>(sizeof(Page) - 1),
                   std::ios::beg);
    char zero = 0;
    fstream_.write(&zero, 1);
    fstream_.flush();
  }
}
} // namespace DiskManager
// ---------- Simple buffer pool manager with LRU replacement ----------