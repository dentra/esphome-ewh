// #include <cstdlib>

#include "ewh_utils.h"

namespace esphome {
namespace ewh {

static int char2int(char ch) {
  if (ch >= '0' && ch <= '9') {
    return ch - '0';
  }
  if (ch >= 'A' && ch <= 'F') {
    return ch - 'A' + 0xA;
  }
  if (ch >= 'a' && ch <= 'f') {
    return ch - 'a' + 0xA;
  }
  return -1;
}

// converts hex string to bytes skip spaces, dots and dashes if exist.
std::vector<uint8_t> from_hex(const std::string &hex) {
  std::vector<uint8_t> res;
  for (const char *ptr = hex.c_str(), *end = ptr + hex.length(); ptr < end; ptr++) {
    if (*ptr == ' ' || *ptr == '.' || *ptr == '-' || *ptr == ':') {
      continue;
    }
    auto byte = char2int(*ptr) << 4;
    ptr++;
    byte += char2int(*ptr);
    res.push_back(byte);
  }
  return res;
}

}  // namespace ewh
}  // namespace esphome
