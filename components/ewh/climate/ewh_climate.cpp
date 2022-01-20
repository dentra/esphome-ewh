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

void EWHClimate::dump_config() {
  LOG_CLIMATE("", "EWH Climate", this);
  EWHComponent::dump_config();
}

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
      PRESET_MODE1, PRESET_MODE2, PRESET_MODE3, PRESET_NO_FROST,
      // PRESET_TIMER,
  });

  // traits.set_supports_action(true);

  return traits;
}

ewh_mode_t::Mode EWHClimate::to_wh_mode_(ClimateMode mode, const std::string &preset) const {
  if (mode == ClimateMode::CLIMATE_MODE_OFF) {
    return ewh_mode_t::MODE_OFF;
  }
  if (preset == PRESET_MODE1) {
    return ewh_mode_t::MODE_700W;
  }
  if (preset == PRESET_MODE2) {
    return ewh_mode_t::MODE_1300W;
  }
  if (preset == PRESET_MODE3) {
    return ewh_mode_t::MODE_2000W;
  }
  if (preset == PRESET_NO_FROST) {
    return ewh_mode_t::MODE_NO_FROST;
  }
  if (preset == PRESET_TIMER) {
    return ewh_mode_t::MODE_700W;
  }
  if (mode == ClimateMode::CLIMATE_MODE_HEAT) {
    return ewh_mode_t::MODE_700W;
  }
  return ewh_mode_t::MODE_OFF;
}

void EWHClimate::control(const ClimateCall &call) {
  if (call.get_target_temperature().has_value()) {
    this->target_temperature = *call.get_target_temperature();
  }

  if (call.get_mode().has_value()) {
    this->mode = *call.get_mode();
  }

  if (call.get_custom_preset().has_value()) {
    this->custom_preset = *call.get_custom_preset();
  }

  auto wh_mode = this->to_wh_mode_(this->mode, *this->custom_preset);

  this->ewh_->set_mode(wh_mode, this->target_temperature);

  // this->publish_state();
}

void EWHClimate::read(const ewh_status_t &status) {
  this->target_temperature = status.target_temperature;
  this->current_temperature = status.current_temperature;

  if (status.state == ewh_state_t::STATE_OFF) {
    this->mode = climate::CLIMATE_MODE_OFF;
    this->action = climate::CLIMATE_ACTION_OFF;
    if (!this->custom_preset.has_value()) {
      this->custom_preset = PRESET_MODE1;
    }
  } else {
    this->mode = climate::CLIMATE_MODE_HEAT;
    // if detect real heating then add ClimateAction::CLIMATE_ACTION_IDLE
    this->action = climate::CLIMATE_ACTION_HEATING;
    if (status.state == ewh_state_t::STATE_700W) {
      this->custom_preset = PRESET_MODE1;
    } else if (status.state == ewh_state_t::STATE_1300W) {
      this->custom_preset = PRESET_MODE2;
    } else if (status.state == ewh_state_t::STATE_2000W) {
      this->custom_preset = PRESET_MODE3;
    } else if (status.state == ewh_state_t::STATE_NO_FROST) {
      this->custom_preset = PRESET_NO_FROST;
    } else if (status.state == ewh_state_t::STATE_TIMER) {
      this->custom_preset = PRESET_TIMER;
    }
  }

  this->publish_state();

  EWHComponent::read(status);
}

// void EWHClimate::read(const ewh_state_t &state) {
//   if (state.state == ewh_state_t::STATE_TIMER) {
//     // timer set outside this component, so update state immediately
//     this->mode = climate::CLIMATE_MODE_HEAT;
//     this->action = climate::CLIMATE_ACTION_HEATING;
//     this->custom_preset = PRESET_TIMER;
//     this->publish_state();
//     return;
//   }

//   bool fail{};
//   if (this->target_temperature != state.target_temperature) {
//     fail = true;
//   } else if (this->mode == climate::CLIMATE_MODE_OFF) {
//     fail = state.state != ewh_state_t::STATE_OFF;
//   } else if (this->custom_preset.has_value()) {
//     auto preset = *this->custom_preset;
//     fail = (preset == PRESET_MODE1 && state.state != ewh_state_t::STATE_700W) ||
//            (preset == PRESET_MODE2 && state.state != ewh_state_t::STATE_1300W) ||
//            (preset == PRESET_MODE3 && state.state != ewh_state_t::STATE_2000W) ||
//            (preset == PRESET_NO_FROST && state.state != ewh_state_t::STATE_NO_FROST) ||
//            (preset == PRESET_TIMER && state.state != ewh_state_t::STATE_TIMER);
//   }

//   if (fail) {
//     auto preset = *this->custom_preset;
//     auto wh_mode = this->to_wh_mode_(this->mode, preset);
//     if (preset != PRESET_TIMER) {
//       this->ewh_->set_mode(wh_mode, this->target_temperature);
//     } else {
//       this->ewh_->set_timer(0, 0, this->target_temperature, wh_mode);
//     }
//   }
// }

}  // namespace ewh
}  // namespace esphome
