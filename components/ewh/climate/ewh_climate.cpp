#include "esphome/core/log.h"

#include "ewh_climate.h"

namespace esphome {
namespace ewh {

static const char *const TAG = "ewh.climate";

static const char *const PRESET_MODE1 = "0.7 kW";
static const char *const PRESET_MODE2 = "1.3 kW";
static const char *const PRESET_MODE3 = "2.0 kW";
static const char *const PRESET_NO_FROST = "No Frost";
static const char *const PRESET_TIMER = "Timer";
static const char *const PRESET_DEFAULT = PRESET_MODE1;

EWHClimate::EWHClimate(EWHApi *api) : EWHComponent(api) {
  this->set_supported_custom_presets({
      PRESET_MODE1, PRESET_MODE2, PRESET_MODE3, PRESET_NO_FROST,
      // PRESET_TIMER,
  });
}

void EWHClimate::dump_config() {
  LOG_CLIMATE("", "Electrolux Water Heater", this);
  LOG_EWH();
  ESP_LOGCONFIG(TAG, "  Idle temperature drop: %u", this->heating_.get_idle_temp_drop());
  LOG_SENSOR("  ", "Power", this->power_);
}

ClimateTraits EWHClimate::traits() {
  auto traits = climate::ClimateTraits();

  traits.add_feature_flags(climate::CLIMATE_SUPPORTS_CURRENT_TEMPERATURE | climate::CLIMATE_SUPPORTS_ACTION);
  traits.set_visual_min_temperature(ewh::MIN_TEMPERATURE);
  traits.set_visual_max_temperature(ewh::MAX_TEMPERATURE);
  traits.set_visual_temperature_step(1);

  traits.set_supported_modes({
      ClimateMode::CLIMATE_MODE_OFF,
      ClimateMode::CLIMATE_MODE_HEAT,
  });

  return traits;
}

ewh_mode_t::Mode EWHClimate::to_wh_mode_(ClimateMode mode, StringRef preset) const {
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
  if (wh_mode == ewh_mode_t::MODE_OFF && call.has_custom_preset() && this->set_custom_preset_(preset)) {
    this->publish_state();
  }
  // set_mode do not return value, so we need to request state
  this->set_timeout(100, [this]() { this->api_->request_state(); });
}

void EWHClimate::on_state(const ewh_state_t &status) {
  ESP_LOGV(TAG, "Got st: %u, tT: %u, cT=%u", status.state, status.target_temperature, status.current_temperature);

  bool has_changes{};
  if (status.target_temperature < ewh::MIN_TEMPERATURE || status.target_temperature > ewh::MAX_TEMPERATURE) {
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
  const bool is_on = status.state != ewh_state_t::STATE_OFF;
  const bool is_heating = this->update_heating_(status);
  if (is_on) {
    mode = climate::CLIMATE_MODE_HEAT;
    action = is_heating ? climate::CLIMATE_ACTION_HEATING : ClimateAction::CLIMATE_ACTION_IDLE;
    if (status.state == ewh_state_t::STATE_700W) {
      preset = PRESET_MODE1;
    } else if (status.state == ewh_state_t::STATE_1300W) {
      preset = PRESET_MODE2;
    } else if (status.state == ewh_state_t::STATE_2000W) {
      preset = PRESET_MODE3;
    } else if (status.state == ewh_state_t::STATE_NO_FROST) {
      preset = PRESET_NO_FROST;
    } else if (status.state == ewh_state_t::STATE_TIMER) {
      preset = PRESET_TIMER;
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

  EWHComponent::on_state(status);
}

void EWHClimate::publish_power_(const ewh_state_t &state) {
  if (this->power_ == nullptr) {
    return;
  }
  float power = 0;
  if (this->heating_.is_heating()) {
    switch (state.state) {
      case ewh_state_t::STATE_1300W:
        power = 1300;
        break;
      case ewh_state_t::STATE_2000W:
        power = 2000;
        break;
      case ewh_state_t::STATE_700W:
      case ewh_state_t::STATE_NO_FROST:
        power = 700;
        break;
      default:
        break;
    }
  }
  // publish on every state to let energy integration grow while power is not changed
  this->power_->publish_state(power);
}

bool EWHClimate::update_heating_(const ewh_state_t &state) {
  // timer waits for the scheduled time, heating is unknown, so do not count it
  const bool is_on = state.state != ewh_state_t::STATE_OFF && state.state != ewh_state_t::STATE_TIMER;
  // no frost mode keeps water just above freezing regardless of target temperature
  const uint8_t target = state.state == ewh_state_t::STATE_NO_FROST ? NO_FROST_TEMPERATURE : state.target_temperature;
  if (is_on && state.state != ewh_state_t::STATE_NO_FROST &&
      (target < ewh::MIN_TEMPERATURE || target > ewh::MAX_TEMPERATURE)) {
    // invalid target temperature, keep previous estimation
    return this->heating_.is_heating();
  }
  return this->heating_.update(is_on, state.current_temperature, target);
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
