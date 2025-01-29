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
  ESP_LOGD("Turn power %s", ONOFF(power));
  this->write_byte(ehu_packet_type_t::PACKET_REQ_SET_POWER, power);
}

void EHUApi::set_preset(uint8_t preset) {
  // TODO write preset
  // this->write_byte(ehu_packet_type_t::PACKET_REQ_SET_PRESET, op);
}

}  // namespace ehu
}  // namespace esphome
