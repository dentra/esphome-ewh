#include "esphome/core/log.h"
#include "esphome/core/helpers.h"

#include "ehu_api.h"

// aa02 0a 00 b6
// aa02 0a 01 b7
// aa02 16 01 c3
// aa02 12 00 be
// aa02 19 32 f7
// aa02 0b 11 c8

namespace esphome {
namespace ehu {

static const char *const TAG = "ehu_api";

using namespace esphome;

void EHUApi::set_power(bool power) {
  ESP_LOGD(TAG, "Turn power to %s", ONOFF(power));
  this->write_byte(ehu_packet_type_t::PACKET_REQ_SET_POWER, power);
}

void EHUApi::set_preset(uint8_t preset) {
  ESP_LOGD(TAG, "Set preset to %u", preset);
  this->write_byte(ehu_packet_type_t::PACKET_REQ_SET_PRESET, preset);
}

void EHUApi::set_speed(uint8_t speed) {
  ESP_LOGD(TAG, "Set speed to %u", speed);
  if (speed == 2) {
    // aa02 19 3c 01 вентилятор среднее
    this->write_byte(ehu_packet_type_t::PACKET_REQ_SET_SPEED_MID, 0x3C);
  } else {
    // aa02 1a 01 c7 вентилятор минимум
    // aa02 1a 03 c9 вентилятор макс
    this->write_byte(ehu_packet_type_t::PACKET_REQ_SET_SPEED, speed);
  }
}

void EHUApi::set_clock(uint8_t hours, uint8_t minutes) {
  if (hours > 23) {
    ESP_LOGW(TAG, "Clock hours must be in [0:23] range");
    return;
  }
  if (minutes > 59) {
    ESP_LOGW(TAG, "Clock minutes must be in [0:59] range");
    return;
  }
  ESP_LOGD(TAG, "Set clock to %02u:%02u", hours, minutes);
  struct {
    uint8_t hours;
    uint8_t minutes;
  } PACKED data{.hours = hours, .minutes = minutes};
  this->write(ehu_packet_type_t::PACKET_REQ_SET_CLOCK, data);
}

}  // namespace ehu
}  // namespace esphome
