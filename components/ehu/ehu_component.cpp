#include "esphome/core/log.h"

#include "ehu_component.h"

namespace esphome {
namespace ehu {

// выкл значка wifi AA 02 03 02 CC
// вкл значок wifi AA 02 03 03 CC

static const char *const TAG = "ehu.component";

static const std::string PRESET__EMPTY = "";
static const std::string PRESET_AUTO = "Auto";              // 01 - AUTO
static const std::string PRESET_HEALTH = "Health";          // 02 - Здоровье
static const std::string PRESET_NIGHT = "Night";            // 03 - NIGHT
static const std::string PRESET_BABY = "Baby";              // 04 - Детский
static const std::string PRESET_FITNESS = "Fitness";        // 06 - фитнес
static const std::string PRESET_YOGA = "Yoga";              // 07 - YOGA
static const std::string PRESET_MEDITATION = "Meditation";  // 08 - Медитация
static const std::string PRESET_PRANA = "Prana";            // 0C - Prana
static const std::string PRESET_MANUAL = "Manual";          // 0F - ручной

#define ALL_PRESETS \
  { \
    PRESET_AUTO, PRESET_HEALTH, PRESET_NIGHT, PRESET_BABY, PRESET_FITNESS, PRESET_YOGA, PRESET_MEDITATION, \
        PRESET_PRANA, PRESET_MANUAL, \
  }

void EHUComponent::dump_config_(const char *TAG) const {
  LOG_SENSOR("  ", "Temperature", this->temperature_);
  LOG_SENSOR("  ", "Humidity", this->humidity_);
  LOG_SWITCH("  ", "Warm Mist", this->warm_mist_);
  LOG_SWITCH("  ", "UV", this->uv_);
  LOG_SWITCH("  ", "Ionizer", this->ionizer_);
  LOG_SWITCH("  ", "Lock", this->lock_);
  LOG_SWITCH("  ", "Mute", this->mute_);
  LOG_BINARY_SENSOR("  ", "Water", this->water_);
  // LOG_FAN("  ", "Fan", this->fan_);
}

void EHUComponent::on_state(const ehu_state_t &state) {
#ifdef USE_TIME
  auto time = this->time_->now();
  if (time.is_valid() && state.clock_hours != time.hour && state.clock_minutes != time.minute) {
    this->defer([this]() {
      auto time = this->time_->now();
      this->api_->set_clock(time.hour, time.minute);
    });
  }
#endif

  this->publish_state_(this->temperature_, state.temperature);
  this->publish_state_(this->humidity_, state.humidity);
  this->publish_state_(this->warm_mist_, state.water_flags & ehu_state_t::WATER_WARM_MIST);
  this->publish_state_(this->uv_, state.water_flags & ehu_state_t::WATER_UV);
  this->publish_state_(this->ionizer_, state.ionizer);
  this->publish_state_(this->lock_, state.lock);
  this->publish_state_(this->mute_, state.mute);
  this->publish_state_(this->water_, !state.water_tank_empty);
  this->publish_fan_state_(state);
  this->publish_state_(this->target_humidity_, state.target_humidity);
  this->publish_state_(this->speed_, state.fan_speed);
  if (this->preset_) {
    this->preset_->publish_state(this->get_preset_(state));
  }
}

void EHUComponent::publish_fan_state_(const ehu_state_t &state) {
  if (this->fan_ == nullptr) {
    return;
  }

  bool has_changes{};

  if (state.power != this->fan_->state) {
    this->fan_->state = state.power;
    has_changes = true;
  }

  if (state.fan_speed != this->fan_->speed) {
    this->fan_->speed = state.fan_speed;
    has_changes = true;
  }

  auto &preset = this->get_preset_(state);
  if (preset != this->fan_->preset_mode) {
    this->fan_->preset_mode = preset;
    has_changes = true;
  }

  if (has_changes) {
    this->fan_->publish_state();
  }
}

const std::string &EHUComponent::get_preset_(const ehu_state_t &state) const {
  switch (state.preset) {
    case ehu_state_t::PRESET_AUTO:
      return PRESET_AUTO;
    case ehu_state_t::PRESET_BABY:
      return PRESET_BABY;
    case ehu_state_t::PRESET_FITNESS:
      return PRESET_FITNESS;
    case ehu_state_t::PRESET_HEALTH:
      return PRESET_HEALTH;
    case ehu_state_t::PRESET_MANUAL:
      return PRESET_MANUAL;
    case ehu_state_t::PRESET_MEDITATION:
      return PRESET_MEDITATION;
    case ehu_state_t::PRESET_NIGHT:
      return PRESET_NIGHT;
    case ehu_state_t::PRESET_PRANA:
      return PRESET_PRANA;
    case ehu_state_t::PRESET_YOGA:
      return PRESET_YOGA;
    default:
      return PRESET__EMPTY;
  }
}

void EHUComponent::write_preset_(const std::string &preset) const {
  if (preset.empty()) {
    return;
  }
  if (preset == PRESET_AUTO) {
    this->api_->set_preset(ehu_state_t::PRESET_AUTO);
  } else if (preset == PRESET_HEALTH) {
    this->api_->set_preset(ehu_state_t::PRESET_HEALTH);
  } else if (preset == PRESET_NIGHT) {
    this->api_->set_preset(ehu_state_t::PRESET_NIGHT);
  } else if (preset == PRESET_BABY) {
    this->api_->set_preset(ehu_state_t::PRESET_BABY);
  } else if (preset == PRESET_FITNESS) {
    this->api_->set_preset(ehu_state_t::PRESET_FITNESS);
  } else if (preset == PRESET_YOGA) {
    this->api_->set_preset(ehu_state_t::PRESET_YOGA);
  } else if (preset == PRESET_MEDITATION) {
    this->api_->set_preset(ehu_state_t::PRESET_MEDITATION);
  } else if (preset == PRESET_PRANA) {
    this->api_->set_preset(ehu_state_t::PRESET_PRANA);
  } else {
    this->api_->set_preset(ehu_state_t::PRESET_MANUAL);
  }
}

void EHUFan::control(const fan::FanCall &call) {
  if (call.get_state().has_value() && !*call.get_state()) {
    this->parent_->api_->set_speed(0);
  }

  if (call.get_speed().has_value()) {
    this->parent_->api_->set_speed(*call.get_speed());
  }

  this->parent_->write_preset_(call.get_preset_mode());
}

fan::FanTraits EHUFan::get_traits() {
  auto traits = fan::FanTraits();
  traits.set_speed(true);
  traits.set_supported_speed_count(3);
  traits.set_supported_preset_modes(ALL_PRESETS);
  return traits;
}

void EHUPreset::setup() { this->traits.set_options(ALL_PRESETS); }

}  // namespace ehu
}  // namespace esphome
