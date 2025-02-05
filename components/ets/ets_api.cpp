#include "esphome/core/log.h"
#include "esphome/core/helpers.h"

#include "ets_api.h"

namespace esphome {
namespace ets {

static const char *const TAG = "ets_api";

void ETSApi::set_mode(bool *state, float target_temp, float air_temp) const {
  if (this->unk0C_ == 0) {
    ESP_LOGW(TAG, "Сurrent state has not been received yet");
    return;
  }
  ets_mode_t mode;
  if (state) {
    mode.set_state(*state);
  }
  mode.target_temp(target_temp);
  mode.air_temp(air_temp);
  mode.unk0C = this->unk0C_;
  this->write_st(mode);
}

}  // namespace ets
}  // namespace esphome
