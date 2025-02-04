#include "esphome/core/log.h"

#include "ehu_component.h"

namespace esphome {
namespace ehu {

// выкл значка wifi AA 02 03 02 CC
// вкл значок wifi AA 02 03 03 CC

static const char *const TAG = "ehu.component";

static const std::string PRESET_AUTO = "Auto";              // 01 - AUTO
static const std::string PRESET_HEALTH = "Health";          // 02 - Здоровье
static const std::string PRESET_NIGHT = "Night";            // 03 - NIGHT
static const std::string PRESET_BABY = "Baby";              // 04 - Детский
static const std::string PRESET_FITNESS = "Fitness";        // 06 - фитнес
static const std::string PRESET_YOGA = "Yoga";              // 07 - YOGA
static const std::string PRESET_MEDITATION = "Meditation";  // 08 - Медитация
static const std::string PRESET_PRANA = "Prana";            // 0C - Prana
static const std::string PRESET_MANUAL = "Manual";          // 0F - ручной

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
  // this->publish_state_(this->target_humidity_, state.target_humidity_);
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

  std::string preset{};
  switch (state.preset) {
    case ehu_state_t::PRESET_AUTO:
      preset = PRESET_AUTO;
      break;
    case ehu_state_t::PRESET_BABY:
      preset = PRESET_BABY;
      break;
    case ehu_state_t::PRESET_FITNESS:
      preset = PRESET_FITNESS;
      break;
    case ehu_state_t::PRESET_HEALTH:
      preset = PRESET_HEALTH;
      break;
    case ehu_state_t::PRESET_MANUAL:
      preset = PRESET_MANUAL;
      break;
    case ehu_state_t::PRESET_MEDITATION:
      preset = PRESET_MEDITATION;
      break;
    case ehu_state_t::PRESET_NIGHT:
      preset = PRESET_NIGHT;
      break;
    case ehu_state_t::PRESET_PRANA:
      preset = PRESET_PRANA;
      break;
    case ehu_state_t::PRESET_YOGA:
      preset = PRESET_YOGA;
      break;
    default:
      // pass
      break;
  }
  if (preset != this->fan_->preset_mode) {
    this->fan_->preset_mode = preset;
    has_changes = true;
  }

  if (has_changes) {
    this->fan_->publish_state();
  }
}

fan::FanTraits EHUFan::get_traits() {
  auto traits = fan::FanTraits();
  traits.set_speed(true);
  traits.set_supported_speed_count(3);
  traits.set_supported_preset_modes({
      PRESET_AUTO,
      PRESET_HEALTH,
      PRESET_NIGHT,
      PRESET_BABY,
      PRESET_FITNESS,
      PRESET_YOGA,
      PRESET_MEDITATION,
      PRESET_PRANA,
      PRESET_MANUAL,
  });
  return traits;
}

void EHUFan::control(const fan::FanCall &call) {
  if (call.get_state().has_value()) {
    this->api_->set_speed(0);
  }

  if (call.get_speed().has_value()) {
    this->api_->set_speed(*call.get_speed());
  }

  auto preset = call.get_preset_mode();
  if (!preset.empty()) {
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
}

}  // namespace ehu
}  // namespace esphome
