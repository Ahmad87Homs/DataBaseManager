#pragma once
#include <cstdint>
#include <cstring>
namespace DiskManager {

constexpr size_t kPageSize = 4096;

enum class PageType : uint16_t {
    HEAP = 1,
    INDEX = 2,
    CATALOG = 3
};

// Abstract base page interface
class IPage {
public:
    virtual ~IPage() = default;

    virtual uint32_t getPageId() const = 0;
    virtual PageType getPageType() const = 0;

    virtual uint16_t getRowCount() const = 0;
    virtual uint16_t getFreeSpacePointer() const = 0;

    // Serialize full page (4KB) to raw buffer
    virtual void toBytes(uint8_t* out) const = 0;

    // Load full page from raw buffer
    virtual void fromBytes(const uint8_t* in) = 0;
};
}