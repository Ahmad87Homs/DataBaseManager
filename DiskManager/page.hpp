#pragma once
#pragma pack(push, 1)
#include "Ipage.hpp"
#include <cstdint>
#include <cstring>

namespace DiskManager {
struct Slot {
    uint16_t offset;
    uint16_t size;
};

constexpr size_t kMaxSlots = 32;
constexpr size_t sizeOfDataArea() {
    return kPageSize - (sizeof(uint32_t) + sizeof(uint16_t) * 4 + sizeof(Slot) * kMaxSlots);
}

class HeapPage : public IPage {
public:
    struct page_header_t {
        uint32_t page_id;
        uint16_t page_type;
        uint16_t row_count;
        uint16_t free_space_ptr;
        uint16_t flags;
    };

    struct slot_directory_t {
        Slot slots[kMaxSlots];
    };

    struct data_area_t {
        uint8_t data[sizeOfDataArea()];
    };
    struct {
          page_header_t header;
          slot_directory_t slot_directory;
          data_area_t data_area;  
    } data;
    HeapPage() { init(0); }

    void init(uint32_t pid, PageType type = PageType::HEAP) {
        data.header.page_id = pid;
         data.header.page_type = static_cast<uint16_t>(type);
         data.header.row_count = 0;
         data.header.free_space_ptr = 0;
         data.header.flags = 0;
        std::memset( data.slot_directory.slots, 0, sizeof( data.slot_directory.slots));
        std::memset( data.data_area.data, 0, sizeof( data.data_area.data));
    }

    // --- Implement IPage interface ---
    uint32_t getPageId() const override { return  data.header.page_id; }
    PageType getPageType() const override { return static_cast<PageType>( data.header.page_type); }
    uint16_t getRowCount() const override { return  data.header.row_count; }
    uint16_t getFreeSpacePointer() const override { return  data.header.free_space_ptr; }

    void toBytes(uint8_t* out) const override {
        std::memcpy(out,  &data, sizeof(this->data));
    }

    void fromBytes(const uint8_t* in) override {
        std::memcpy(&data, in, sizeof(this->data));
    }
};
#pragma pack(pop)

} // namespace DiskManager
