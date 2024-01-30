#include "esphome/core/log.h"

#include "bwh_climate.h"

namespace esphome {
namespace bwh {

static const char *const TAG = "bwh.climate";

static const std::string PRESET_MODE2 = "1300 W";
static const std::string PRESET_MODE3 = "2000 W";
static const std::string &PRESET_DEFAULT = PRESET_MODE2;

void BWHClimate::dump_config() { LOG_CLIMATE("", "Ballu Water Heater", this); }

ClimateTraits BWHClimate::traits() {
  auto traits = climate::ClimateTraits();

  traits.set_supports_current_temperature(true);
  traits.set_visual_min_temperature(bwh::MIN_TEMPERATURE);
  traits.set_visual_max_temperature(bwh::MAX_TEMPERATURE);
  traits.set_visual_temperature_step(1);

  traits.set_supported_modes({
      ClimateMode::CLIMATE_MODE_OFF,
      ClimateMode::CLIMATE_MODE_HEAT,
  });

  traits.set_supported_custom_presets({
      PRESET_MODE2, PRESET_MODE3,
      // PRESET_TIMER,
  });

  // traits.set_supports_action(true);

  return traits;
}

bwh_mode_t::Mode BWHClimate::to_wh_mode_(ClimateMode mode, const std::string &preset) const {
  if (mode == ClimateMode::CLIMATE_MODE_OFF) {
    return bwh_mode_t::MODE_OFF;
  }
  if (preset == PRESET_MODE2) {
    return bwh_mode_t::MODE_1300W;
  }
  if (preset == PRESET_MODE3) {
    return bwh_mode_t::MODE_2000W;
  }
  if (mode == ClimateMode::CLIMATE_MODE_HEAT) {
    return bwh_mode_t::MODE_1300W;
  }
  return bwh_mode_t::MODE_OFF;
}

void BWHClimate::control(const ClimateCall &call) {
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

  this->api_->set_mode(wh_mode, this->target_temperature);

  // this->publish_state();
}

void BWHClimate::on_state(const bwh_state_t &status) {
  this->target_temperature = status.target_temperature;
  this->current_temperature = status.current_temperature;

  if (status.state == bwh_state_t::STATE_OFF) {
    this->mode = climate::CLIMATE_MODE_OFF;
    this->action = climate::CLIMATE_ACTION_OFF;
    if (!this->custom_preset.has_value()) {
      this->custom_preset = PRESET_MODE2;
    }
  } else {
    this->mode = climate::CLIMATE_MODE_HEAT;
    // if detect real heating then add ClimateAction::CLIMATE_ACTION_IDLE
    this->action = climate::CLIMATE_ACTION_HEATING;
    if (status.state == bwh_state_t::STATE_1300W) {
      this->custom_preset = PRESET_MODE2;
    } else if (status.state == bwh_state_t::STATE_2000W) {
      this->custom_preset = PRESET_MODE3;
    } else {
      ESP_LOGW(TAG, "Unknown state %02X", status.state);
    }
  }

  this->publish_state();

  BWHComponent::on_state(status);
}

// void BWHClimate::read(const ewh_state_t &state) {
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

}  // namespace bwh
}  // namespace esphome
