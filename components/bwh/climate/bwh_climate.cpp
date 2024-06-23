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

  traits.set_supports_action(true);

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
  const auto mode = call.get_mode().value_or(this->mode);
  const auto preset = call.get_custom_preset().value_or(*this->custom_preset);
  const auto wh_mode = this->to_wh_mode_(mode, preset);
  const auto temp = call.get_target_temperature().value_or(this->target_temperature);
  if (std::isnan(temp)) {
    ESP_LOGW(TAG, "No target temperature detected");
  } else {
    this->api_->set_mode(wh_mode, temp);
  }
  // special case to allow achange present when is off
  if (wh_mode == bwh_mode_t::MODE_OFF && call.get_custom_preset().has_value()) {
    this->custom_preset = preset;
  }
  // is this actual for BWH?
  this->set_timeout(100, [this]() { this->api_->request_state(); });
}

void BWHClimate::on_state(const bwh_state_t &status) {
  bool has_changes{};
  if (status.target_temperature < bwh::MIN_TEMPERATURE || status.target_temperature > bwh::MAX_TEMPERATURE) {
    ESP_LOGW(TAG, "Target temperarue is out of range %u", status.target_temperature);
    this->target_temperature = NAN;
  } else if (std::isnan(this->target_temperature) ||
             static_cast<uint8_t>(this->target_temperature) != status.target_temperature) {
    this->target_temperature = status.target_temperature;
    has_changes = true;
  }

  if (std::isnan(this->current_temperature) ||
      static_cast<uint8_t>(this->current_temperature) != status.current_temperature) {
    this->current_temperature = status.current_temperature;
    has_changes = true;
  }

  auto mode = climate::CLIMATE_MODE_OFF;
  auto action = climate::CLIMATE_ACTION_OFF;
  auto preset = *this->custom_preset;
  if (status.state != bwh_state_t::STATE_OFF) {
    mode = climate::CLIMATE_MODE_HEAT;
    const bool is_heating =
        (static_cast<int>(status.target_temperature) - static_cast<int>(status.current_temperature)) > 1;
    action = is_heating ? climate::CLIMATE_ACTION_HEATING : ClimateAction::CLIMATE_ACTION_IDLE;
    if (status.state == bwh_state_t::STATE_1300W) {
      preset = PRESET_MODE2;
    } else if (status.state == bwh_state_t::STATE_2000W) {
      preset = PRESET_MODE3;
    } else {
      ESP_LOGW(TAG, "Unknown state %02X", status.state);
    }
  }

  if (mode != this->mode) {
    this->mode = mode;
    has_changes = true;
  }
  if (action != this->action) {
    this->action = action;
    has_changes = true;
  }
  if (preset != *this->custom_preset) {
    this->custom_preset = preset;
    has_changes = true;
  }

  if (has_changes) {
    this->publish_state();
  }

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
