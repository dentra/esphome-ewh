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
  struct {
    uint8_t power;
  } op{.power = power};
  this->write(rka_api::PACKET_REQ_SET_COMMAND, op);
}

void EHUApi::set_preset(uint8_t preset) {
  struct {
    uint8_t preset;
  } op{.preset = preset};
  // TODO write preset
  // this->write(rka_api::PACKET_REQ_SET_COMMAND, op);
}

}  // namespace ehu
}  // namespace esphome
