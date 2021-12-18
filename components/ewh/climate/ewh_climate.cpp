#include "esphome/core/log.h"

#include "ewh_climate.h"

namespace esphome {
namespace ewh {

static const char *const TAG = "ewh.climate";

static const std::string PRESET_MODE1 = "700 W";
static const std::string PRESET_MODE2 = "1300 W";
static const std::string PRESET_MODE3 = "2000 W";
static const std::string PRESET_NO_FROST = "No Frost";
static const std::string PRESET_TIMER = "Timer";

ClimateTraits EWHClimate::traits() {
  auto traits = climate::ClimateTraits();

  traits.set_supports_current_temperature(true);
  traits.set_visual_min_temperature(MIN_TEMPERATURE);
  traits.set_visual_max_temperature(MAX_TEMPERATURE);
  traits.set_visual_temperature_step(1);

  traits.set_supported_modes({
      ClimateMode::CLIMATE_MODE_OFF,
      ClimateMode::CLIMATE_MODE_HEAT,
  });

  traits.set_supported_custom_presets({
      PRESET_MODE1,
      PRESET_MODE2,
      PRESET_MODE3,
      PRESET_NO_FROST,
      PRESET_TIMER,
  });

  // traits.set_supports_action(true);

  return traits;
}

void EWHClimate::control(const ClimateCall &call) {
  ewh_mode_t wh_mode;
  wh_mode.temperature =
      call.get_target_temperature().has_value() ? *call.get_target_temperature() : this->target_temperature;

  const auto mode = call.get_mode().has_value() ? *call.get_mode() : this->mode;
  const auto preset = call.get_custom_preset().has_value() ? *call.get_custom_preset() : *this->custom_preset;

  if (mode == ClimateMode::CLIMATE_MODE_OFF) {
    wh_mode.mode = ewh_mode_t::MODE_OFF;
  } else if (preset == PRESET_MODE1) {
    wh_mode.mode = ewh_mode_t::MODE_700W;
  } else if (preset == PRESET_MODE2) {
    wh_mode.mode = ewh_mode_t::MODE_1300W;
  } else if (preset == PRESET_MODE3) {
    wh_mode.mode = ewh_mode_t::MODE_2000W;
  } else if (preset == PRESET_NO_FROST) {
    wh_mode.mode = ewh_mode_t::MODE_NO_FROST;
  } else if (mode == ClimateMode::CLIMATE_MODE_HEAT) {
    wh_mode.mode = ewh_mode_t::MODE_700W;
  }

  if (preset != PRESET_TIMER) {
    this->send_control_state_(ewh_mode_t::SET_OPERATION, &wh_mode, sizeof(wh_mode));
  } else {
    ewh_timer_t wh_timer;
    wh_timer.mode = wh_mode.mode;
    wh_timer.temperature = wh_mode.temperature;
    this->send_control_state_(ewh_timer_t::SET_OPERATION, &wh_timer, sizeof(wh_timer));
  }

  // this->publish_state();
}

void EWHClimate::read(const ewh_state_t &state) {
  this->target_temperature = state.target_temperature;
  this->current_temperature = state.current_temperature;

  if (state.state == ewh_state_t::STATE_OFF) {
    this->mode = climate::CLIMATE_MODE_OFF;
    this->action = climate::CLIMATE_ACTION_OFF;
  } else {
    this->mode = climate::CLIMATE_MODE_HEAT;
    // if detect real heating then add ClimateAction::CLIMATE_ACTION_IDLE
    this->action = climate::CLIMATE_ACTION_HEATING;
    if (state.state == ewh_state_t::STATE_700W) {
      this->custom_preset = PRESET_MODE1;
    } else if (state.state == ewh_state_t::STATE_1300W) {
      this->custom_preset = PRESET_MODE2;
    } else if (state.state == ewh_state_t::STATE_2000W) {
      this->custom_preset = PRESET_MODE3;
    } else if (state.state == ewh_state_t::STATE_NO_FROST) {
      this->custom_preset = PRESET_NO_FROST;
    } else if (state.state == ewh_state_t::STATE_TIMER) {
      this->custom_preset = PRESET_TIMER;
    }
  }

  this->publish_state();

  EWHComponent::read(state);
}

}  // namespace ewh
}  // namespace esphome
