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

static const std::string LED_PRESET_OFF = "Off";        // 00 - выключение
static const std::string LED_PRESET_RANDOM = "Random";  // 01 - случайный цвет
static const std::string LED_PRESET_BLUE = "Blue";      // 02 - синий
static const std::string LED_PRESET_GREEN = "Green";    // 03 - зеленый
static const std::string LED_PRESET_WHITE = "White";    // 04 - белый.

#define ALL_FAN_PRESETS \
  { \
    PRESET_AUTO, PRESET_HEALTH, PRESET_NIGHT, PRESET_BABY, PRESET_FITNESS, PRESET_YOGA, PRESET_MEDITATION, \
        PRESET_PRANA, PRESET_MANUAL, \
  }

#define ALL_LED_PRESETS \
  { LED_PRESET_OFF, LED_PRESET_RANDOM, LED_PRESET_BLUE, LED_PRESET_GREEN, LED_PRESET_WHITE }

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
  LOG_NUMBER("  ", "Target Humidity", this->target_humidity_);
  LOG_NUMBER("  ", "Speed", this->fan_speed_);
  LOG_SELECT("  ", "Preset", this->fan_preset_);
  LOG_NUMBER("  ", "LED Brightness", this->led_brightness_);
  LOG_SWITCH("  ", "LED Top", this->led_top_);
  LOG_SWITCH("  ", "LED Bottom", this->led_bottom_);
  LOG_SELECT("  ", "LED Preset", this->led_preset_);
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
  this->publish_state_(this->fan_speed_, state.fan_speed);
  if (this->fan_preset_) {
    auto &preset = this->get_fan_preset_(state);
    if (!preset.empty()) {
      this->fan_preset_->publish_state(preset);
    }
  }
  this->publish_state_(this->led_brightness_, state.led_brightness);
  this->publish_state_(this->led_top_, state.led_mode & ehu_state_t::LED_MODE_TOP);
  this->publish_state_(this->led_bottom_, state.led_mode & ehu_state_t::LED_MODE_BOTTOM);
  if (this->led_preset_) {
    auto &preset = this->get_led_preset_(state);
    if (!preset.empty()) {
      this->led_preset_->publish_state(preset);
    }
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

  auto &preset = this->get_fan_preset_(state);
  if (preset != this->fan_->preset_mode) {
    this->fan_->preset_mode = preset;
    has_changes = true;
  }

  if (has_changes) {
    this->fan_->publish_state();
  }
}

const std::string &EHUComponent::get_fan_preset_(const ehu_state_t &state) const {
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
      ESP_LOGW(TAG, "Unknown fan preset %u", state.led_preset);
      return PRESET__EMPTY;
  }
}

void EHUComponent::write_fan_preset_(const std::string &preset) const {
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

const std::string &EHUComponent::get_led_preset_(const ehu_state_t &state) const {
  switch (state.led_preset) {
    case ehu_state_t::LED_PRESET_OFF:
      return LED_PRESET_OFF;
    case ehu_state_t::LED_PRESET_RANDOM:
      return LED_PRESET_RANDOM;
    case ehu_state_t::LED_PRESET_BLUE:
      return LED_PRESET_BLUE;
    case ehu_state_t::LED_PRESET_GREEN:
      return LED_PRESET_GREEN;
    case ehu_state_t::LED_PRESET_WHITE:
      return LED_PRESET_WHITE;
    default:
      ESP_LOGW(TAG, "Unknown LED preset %u", state.led_preset);
      return PRESET__EMPTY;
  }
}

void EHUFan::control(const fan::FanCall &call) {
  if (call.get_state().has_value() && !*call.get_state()) {
    this->parent_->api_->set_speed(0);
  }

  if (call.get_speed().has_value()) {
    this->parent_->api_->set_speed(*call.get_speed());
  }

  this->parent_->write_fan_preset_(call.get_preset_mode());
}

fan::FanTraits EHUFan::get_traits() {
  auto traits = fan::FanTraits();
  traits.set_speed(true);
  traits.set_supported_speed_count(3);
  traits.set_supported_preset_modes(ALL_FAN_PRESETS);
  return traits;
}

void EHUFanPreset::setup() { this->traits.set_options(ALL_FAN_PRESETS); }

void EHULedPreset::setup() { this->traits.set_options(ALL_LED_PRESETS); }

void EHULedPreset::control(const std::string &value) {
  if (value.empty()) {
    return;
  }
  if (value == LED_PRESET_OFF) {
    this->parent_->api_->set_led_preset(ehu_state_t::LED_PRESET_OFF);
  } else if (value == LED_PRESET_RANDOM) {
    this->parent_->api_->set_led_preset(ehu_state_t::LED_PRESET_RANDOM);
  } else if (value == LED_PRESET_BLUE) {
    this->parent_->api_->set_led_preset(ehu_state_t::LED_PRESET_BLUE);
  } else if (value == LED_PRESET_GREEN) {
    this->parent_->api_->set_led_preset(ehu_state_t::LED_PRESET_GREEN);
  } else if (value == LED_PRESET_WHITE) {
    this->parent_->api_->set_led_preset(ehu_state_t::LED_PRESET_WHITE);
  } else {
    ESP_LOGW(TAG, "Unknown led preset %s", value.c_str());
  }
}

}  // namespace ehu
}  // namespace esphome
