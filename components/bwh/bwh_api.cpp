#include "esphome/core/log.h"
#include "esphome/core/helpers.h"

#include "bwh_api.h"

namespace esphome {
namespace bwh {

static const char *const TAG = "bwh_api";

using namespace esphome;

void BWHApi::set_mode(bwh_mode_t::Mode mode, uint8_t target_temperature) {
  this->set_mode(bwh_mode_t{
      .mode = mode,
      .temperature = target_temperature,
  });
}

void BWHApi::set_mode(const bwh_mode_t &mode) {
  ESP_LOGD(TAG, "Set mode to %u, temperature %u", mode.mode, mode.temperature);
  this->write_op(mode);
}

}  // namespace bwh
}  // namespace esphome
