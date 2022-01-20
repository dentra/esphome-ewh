#pragma once

#include <vector>
#include <string>

namespace esphome {
namespace ewh {
// converts hex string to bytes skip spaces, dots and dashes if exist.
std::vector<uint8_t> from_hex(const std::string &hex);

inline uint8_t crc8(uint8_t crc, const void *data, uint32_t size) {
  auto data8 = static_cast<const uint8_t *>(data);
  while (size--) {
    crc += *data8++;
  }
  return crc;
}

inline uint8_t crc8(const uint8_t frame_header, const uint8_t frame_size, const void *data, uint32_t size) {
  return crc8(frame_header + frame_size, data, size);
}

inline uint8_t crc8(const uint8_t frame_header, const uint8_t frame_size, const uint8_t frame_type, const void *data,
                    uint32_t size) {
  return crc8(frame_header + frame_size + frame_type, data, size);
}

}  // namespace ewh
}  // namespace esphome
