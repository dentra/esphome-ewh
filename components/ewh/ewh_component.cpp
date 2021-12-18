#include "esphome/core/log.h"
#include "esphome/core/helpers.h"

#include "ewh_component.h"

namespace esphome {
namespace ewh {

static const char *const TAG = "ewh.component";

// converts hex string to bytes skip spaces, dots and dashes if exist.
std::vector<uint8_t> from_hex(const std::string &hex) {
  std::vector<uint8_t> res;
  for (unsigned int i = 0, l = hex.length(); i < l; i++) {
    const char *ptr = hex.c_str() + i;
    if (*ptr == ' ' || *ptr == '.' || *ptr == '-' || *ptr == ':') {
      continue;
    }
    char cc[3] = {*ptr, *(ptr + 1), 0};
    uint8_t byte = (uint8_t) strtol(cc, nullptr, 16);
    res.push_back(byte);

    i++;
  }
  return res;
}

void EWHComponent::exec(uint8_t cmd, const std::vector<uint8_t> &data) {
  this->send_packet_(static_cast<PacketType>(cmd), data.data(), data.size());
}

void EWHComponent::exec(const std::string &datastr) {
  std::vector<uint8> data = from_hex(datastr);
  if (data.size() == 0) {
    ESP_LOGW(TAG, "Empty command");
    return;
  }
  ESP_LOGD(TAG, "API command: %s", hexencode(data.data(), data.size()).c_str());
  this->send_packet_(static_cast<PacketType>(*data.data()), data.data() + 1, data.size() - 1);
}

void EWHComponent::timer(const int hours, const int minutes, const int temperature) {
  if (hours < 0 || hours > 23) {
    ESP_LOGW(TAG, "Timer hours must be in [0:23] range");
  }
  if (minutes < 0 || minutes > 59) {
    ESP_LOGW(TAG, "Timer minutes must be in [0:23] range");
  }
  if (temperature < MIN_TEMPERATURE || minutes > MAX_TEMPERATURE) {
    ESP_LOGW(TAG, "Timer temperature must be in [%d:%d] range", MIN_TEMPERATURE, MAX_TEMPERATURE);
  }
  this->send_set_timer(hours, minutes, ewh_mode_t::MODE_700W, temperature);
}

void EWHComponent::sync_clock() {
  auto t = this->time_->now();
  if (t.is_valid()) {
    this->send_set_clock(t.hour, t.minute);
  }
}

void EWHComponent::read(const ewh_state_t &state) {
  if (this->bst_ != nullptr) {
    this->bst_->publish_state(state.bst.state != 0);
  }
  if (this->clock_ != nullptr) {
    this->clock_->publish_state(str_snprintf("%02u:%02u", 5, state.clock_hours, state.clock_minutes));
  }
  if (this->timer_ != nullptr) {
    this->timer_->publish_state(str_snprintf("%02u:%02u", 5, state.timer_hours, state.timer_minutes));
  }
}

}  // namespace ewh
}  // namespace esphome
