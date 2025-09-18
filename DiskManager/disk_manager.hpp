#pragma once
#include "page.hpp"
#include <map>
#include <fstream>
#include <optional>
#include <string>
namespace DiskManager {

// ---------- Disk manager ----------
class DiskManager {
public:
  explicit DiskManager(const std::string &filename);

  ~DiskManager();
  bool readPage(uint32_t page_id, IPage &page_out);
  // write (overwrite) page
  void writePage(uint32_t page_id, const IPage &page);

  // ensure page exists (zero-initialize if absent)
  void ensurePageExists(uint32_t page_id);

private:
    std::string file_name;
    std::fstream fstream_;
};

} // namespace DiskManager