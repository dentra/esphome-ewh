#include "esphome/core/log.h"

#include "ets_climate.h"

namespace esphome {
namespace ets {

static const char *const TAG = "ets_climate";

void ETSClimate::dump_config() {
  LOG_CLIMATE("", "Electrolux Thermotronic Smart", this);
  this->dump_config_(TAG);
}

ClimateTraits ETSClimate::traits() {
  auto traits = climate::ClimateTraits();

  traits.set_supports_current_temperature(true);
  traits.set_visual_min_temperature(5.0f);
  traits.set_visual_max_temperature(45.0f);
  traits.set_visual_temperature_step(0.1f);

  traits.set_supported_modes({
      ClimateMode::CLIMATE_MODE_OFF,
      ClimateMode::CLIMATE_MODE_HEAT,
  });

  return traits;
}

void ETSClimate::control(const ClimateCall &call) {
  if (call.get_target_temperature().has_value()) {
    this->target_temperature = *call.get_target_temperature();
    this->api_->set_mode(nullptr, this->target_temperature, this->current_temperature);
  }

  if (call.get_mode().has_value()) {
    this->mode = *call.get_mode();
    bool mode = this->mode != ClimateMode::CLIMATE_MODE_OFF;
    this->api_->set_mode(&mode, this->target_temperature, this->current_temperature);
  }
}

void ETSClimate::on_state(const ets_state_t &state) {
  this->api_->init_unk0C(state.unk0C);

  this->target_temperature = state.target_temp();
  this->current_temperature = state.air_temp();
  this->mode = state.is_off() ? climate::CLIMATE_MODE_OFF : climate::CLIMATE_MODE_HEAT;
  this->publish_state();

  if (this->floor_temp_) {
    float floor_temp = state.floor_temp();
    if (floor_temp > -10.0f) {
      this->floor_temp_->publish_state(floor_temp);
    }
  }
}

}  // namespace ets
}  // namespace esphome
