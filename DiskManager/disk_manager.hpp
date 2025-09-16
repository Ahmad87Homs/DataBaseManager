#pragma once
#include "page.hpp"
#include <map>
namespace DiskManager {

// ---------- Disk manager ----------
class DiskManager {
public:
  DiskManager(const std::string &filename);

  ~DiskManager();
  bool readPage(uint32_t page_id, Page &page_out);
  // write (overwrite) page
  void writePage(uint32_t page_id, const Page &page);

  // ensure page exists (zero-initialize if absent)
  void ensurePageExists(uint32_t page_id);

private:
  std::string file_name;
  std::fstream fstream_;
};

} // namespace DiskManager