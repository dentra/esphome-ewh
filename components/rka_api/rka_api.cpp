#include "esphome/core/log.h"
#include "rka_api.h"

namespace esphome {
namespace rka_api {

static const char *const TAG = "rka_api";

namespace internal {
const char *const TAG_API = rka_api::TAG;
}

void RKAListenerBase::on_frame(const rka_any_frame_t &frame, size_t size) {
  if (frame.type == rka_data_t::RSP_FRAME_TYPE) {
    if (this->check_packet_size_(size, sizeof(rka_data_t))) {
      this->on_data(*reinterpret_cast<const rka_data_t *>(frame.data));
    }
  } else if (frame.type == rka_dev_type_t::RSP_FRAME_TYPE) {
    if (this->check_packet_size_(size, sizeof(rka_dev_type_t))) {
      this->on_dev_type(*reinterpret_cast<const rka_dev_type_t *>(frame.data));
    }
  } else if (frame.type == rka_result_t::RSP_FRAME_TYPE) {
    if (this->check_packet_size_(size, sizeof(rka_result_t))) {
      this->on_result(*reinterpret_cast<const rka_result_t *>(frame.data));
    }
  } else if (frame.type == rka_error_t::RSP_FRAME_TYPE) {
    if (this->check_packet_size_(size, sizeof(rka_error_t))) {
      this->on_error(*reinterpret_cast<const rka_error_t *>(frame.data));
    }
  } else {
    ESP_LOGW(TAG, "Unknown packet: %02X: %s", frame.type, format_hex_pretty(frame.data, size).c_str());
  }
}

bool RKAListenerBase::check_packet_size_(size_t frame_size, size_t expected_data_size) const {
  size_t actual = frame_size - sizeof(rka_any_frame_t::type);
  if (expected_data_size == actual) {
    return true;
  }
  ESP_LOGW(TAG, "Invalid packet size, expected: %zu, actual: %zu", expected_data_size, actual);
  return false;
}

}  // namespace rka_api
}  // namespace esphome
