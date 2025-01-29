#include "esphome/core/log.h"

#include "ehu_fan.h"

namespace esphome {
namespace ehu {

static const char *const TAG = "ehu.fan";

static const std::string PRESET_AUTO = "Auto";              // 01 - AUTO
static const std::string PRESET_HEALTH = "Health";          // 02 - Здоровье
static const std::string PRESET_NIGHT = "Night";            // 03 - NIGHT
static const std::string PRESET_BABY = "Baby";              // 04 - Детский
static const std::string PRESET_FITNESS = "Fitness";        // 06 - фитнес
static const std::string PRESET_YOGA = "Yoga";              // 07 - YOGA
static const std::string PRESET_MEDITATION = "Meditation";  // 08 - Медитация
static const std::string PRESET_MANUAL = "Manual";          // 0F - ручной

void EHUFan::dump_config() { LOG_FAN("", "Electrolux Humidifier", this); }

fan::FanTraits EHUFan::get_traits() {
  auto traits = fan::FanTraits();
  traits.set_supported_preset_modes({
      PRESET_AUTO,
      PRESET_HEALTH,
      PRESET_NIGHT,
      PRESET_BABY,
      PRESET_FITNESS,
      PRESET_YOGA,
      PRESET_MEDITATION,
      PRESET_MANUAL,
  });
  return traits;
}

void EHUFan::control(const fan::FanCall &call) {
  if (call.get_state().has_value()) {
    this->api_->set_power(*call.get_state());
  }

  auto preset = this->preset_mode;
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
    } else {
      this->api_->set_preset(ehu_state_t::PRESET_MANUAL);
    }
  }
}

void EHUFan::on_state(const ehu_state_t &status) {
  bool has_changes{};

  if (status.power != this->state) {
    this->state = status.power;
    has_changes = true;
  }

  std::string preset{};
  switch (status.preset_mode) {
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
    case ehu_state_t::PRESET_YOGA:
      preset = PRESET_YOGA;
      break;
    default:
      preset = "Unknown";
      break;
  }
  if (preset != this->preset_mode) {
    this->preset_mode = preset;
    has_changes = true;
  }

  if (has_changes) {
    this->publish_state();
  }

#ifdef USE_SENSOR
  if (this->temperature_) {
    this->temperature_->publish_state(status.temperature);
  }
  if (this->humidity_) {
    this->humidity_->publish_state(status.humidity);
  }
#endif

#ifdef USE_SWITCH
  if (this->warm_mist_) {
    this->warm_mist_->publish_state(status.water_flags & ehu_state_t::WATER_WARM_MIST);
  }
  if (this->uv_) {
    this->uv_->publish_state(status.water_flags & ehu_state_t::WATER_UV);
  }
  if (this->ionizer_) {
    this->ionizer_->publish_state(status.ionizer);
  }
  if (this->lock_) {
    this->lock_->publish_state(status.lock);
  }
  if (this->sound_) {
    this->sound_->publish_state(status.sound);
  }
#endif

  EHUComponent::on_state(status);
}

}  // namespace ehu
}  // namespace esphome
