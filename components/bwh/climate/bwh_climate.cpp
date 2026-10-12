#include "esphome/core/log.h"

#include "bwh_climate.h"

namespace esphome {
namespace bwh {

static const char *const TAG = "bwh.climate";

static const char *const PRESET_MODE2 = "1.3 kW";
static const char *const PRESET_MODE3 = "2.0 kW";
static const char *const PRESET_DEFAULT = PRESET_MODE2;

BWHClimate::BWHClimate(BWHApi *api) : BWHComponent(api) {
  this->set_supported_custom_presets({
      PRESET_MODE2, PRESET_MODE3,
      // PRESET_TIMER,
  });
}

void BWHClimate::dump_config() {
  LOG_CLIMATE("", "Ballu Water Heater", this);
  ESP_LOGCONFIG(TAG, "  Idle temperature drop: %u", this->heating_.get_idle_temp_drop());
  LOG_SENSOR("  ", "Power", this->power_);
}

ClimateTraits BWHClimate::traits() {
  auto traits = climate::ClimateTraits();

  traits.add_feature_flags(climate::CLIMATE_SUPPORTS_CURRENT_TEMPERATURE | climate::CLIMATE_SUPPORTS_ACTION);
  traits.set_visual_min_temperature(bwh::MIN_TEMPERATURE);
  traits.set_visual_max_temperature(bwh::MAX_TEMPERATURE);
  traits.set_visual_temperature_step(1);

  traits.set_supported_modes({
      ClimateMode::CLIMATE_MODE_OFF,
      ClimateMode::CLIMATE_MODE_HEAT,
  });

  return traits;
}

bwh_mode_t::Mode BWHClimate::to_wh_mode_(ClimateMode mode, StringRef preset) const {
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
  const auto preset = call.has_custom_preset() ? call.get_custom_preset() : this->get_custom_preset();
  const auto wh_mode = this->to_wh_mode_(mode, preset);
  const auto temp = call.get_target_temperature().value_or(this->target_temperature);
  if (std::isnan(temp)) {
    ESP_LOGW(TAG, "No target temperature detected");
  } else {
    this->api_->set_mode(wh_mode, temp);
  }
  // special case to allow achange present when is off
  if (wh_mode == bwh_mode_t::MODE_OFF && call.has_custom_preset() && this->set_custom_preset_(preset)) {
    this->publish_state();
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
  const char *preset = nullptr;
  const bool is_on = status.state != bwh_state_t::STATE_OFF;
  const bool is_heating = this->update_heating_(status);
  if (is_on) {
    mode = climate::CLIMATE_MODE_HEAT;
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
  if (preset != nullptr && this->set_custom_preset_(preset)) {
    has_changes = true;
  }

  if (has_changes) {
    this->publish_state();
  }

  this->publish_power_(status);

  BWHComponent::on_state(status);
}

void BWHClimate::publish_power_(const bwh_state_t &state) {
  if (this->power_ == nullptr) {
    return;
  }
  float power = 0;
  if (this->heating_.is_heating()) {
    if (state.state == bwh_state_t::STATE_1300W) {
      power = 1300;
    } else if (state.state == bwh_state_t::STATE_2000W) {
      power = 2000;
    }
  }
  // publish on every state to let energy integration grow while power is not changed
  this->power_->publish_state(power);
}

bool BWHClimate::update_heating_(const bwh_state_t &state) {
  const bool is_on = state.state != bwh_state_t::STATE_OFF;
  if (is_on && (state.target_temperature < bwh::MIN_TEMPERATURE || state.target_temperature > bwh::MAX_TEMPERATURE)) {
    // invalid target temperature, keep previous estimation
    return this->heating_.is_heating();
  }
  return this->heating_.update(is_on, state.current_temperature, state.target_temperature);
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
