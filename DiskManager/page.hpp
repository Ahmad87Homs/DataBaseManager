#include <array>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <list>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#pragma once
#pragma pack(push, 1)

namespace DiskManager {
// Basic constants
constexpr size_t kPageSize = 4096;
constexpr size_t kMaxSlots = 64; // example (adjustable)
constexpr size_t kSlotEntrySize =
    sizeof(uint16_t) /*offset*/ + sizeof(uint16_t) /*length*/;

// helper: compute data area size
constexpr std::size_t sizeOfDataArea() {
  // reserve: page_id (4), page_type (2), row_count (2), free_ptr (2), reserved
  // (2) + slots
  constexpr std::size_t headerSize = sizeof(uint32_t) + sizeof(uint16_t) +
                                     sizeof(uint16_t) + sizeof(uint16_t) +
                                     sizeof(uint16_t);
  return kPageSize - (headerSize + (kMaxSlots * kSlotEntrySize));
}
constexpr std::size_t startIndexOfDataArea() {
  constexpr std::size_t headerSize = sizeof(uint32_t) + sizeof(uint16_t) +
                                     sizeof(uint16_t) + sizeof(uint16_t) +
                                     sizeof(uint16_t);
  return headerSize + (kMaxSlots * kSlotEntrySize);
}

// slot directory entry
struct Slot {
  uint16_t offset;
  uint16_t length;
};

// page types
enum class PageType : uint16_t {
  UNUSED = 0,
  HEAP = 1,
  INDEX = 2,
  OVERFLOW = 3
};

struct Page {
  uint32_t page_id;   // page id
  uint16_t page_type; // PageType
  uint16_t row_count; // number of rows/slots used
  uint16_t
      free_space_ptr;    // offset within data[] where next row will be appended
  uint16_t flags;        // reserved
  Slot slots[kMaxSlots]; // slot directory (offset relative to start of data[])
  uint8_t data[sizeOfDataArea()]; // payload area

  // initialize page
  void init(uint32_t pid, PageType type = PageType::HEAP) {
    page_id = pid;
    page_type = static_cast<uint16_t>(type);
    row_count = 0;
    free_space_ptr = 0; // relative to data[]
    flags = 0;
    // zero slots and data for safety
    std::memset(slots, 0, sizeof(slots));
    std::memset(data, 0, sizeof(data));
  }
};
#pragma pack(pop)

} // namespace DiskManager
