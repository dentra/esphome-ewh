#pragma once

#include "esphome/core/component.h"

#include "esphome/components/sensor/sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/switch/switch.h"
#include "esphome/components/fan/fan.h"
#include "esphome/components/number/number.h"
#include "esphome/components/select/select.h"

#ifdef USE_TIME
#include "esphome/components/time/real_time_clock.h"
#endif

#include "../rka_api/rka_component.h"
#include "ehu_api.h"

namespace esphome {
namespace ehu {

using EHUComponentBase = rka_api::RKAComponent<rka_api::rka_dev_type_t::EHU, EHUListener, EHUApi, PollingComponent>;

class EHUComponent : public EHUComponentBase {
  template<uint16_t cmd_v> friend class EHUCommandComponent;
  friend class EHUFan;
  friend class EHUFanPreset;
  friend class EHULedPreset;

 public:
  explicit EHUComponent(EHUApi *api) : EHUComponentBase(api) {}
  void on_state(const ehu_state_t &state) override;

  void update() override { this->api_->request_state_ex(); }

#ifdef USE_TIME
  void set_time_id(esphome::time::RealTimeClock *time) { this->time_ = time; }
#endif

  void set_temperature_sensor(sensor::Sensor *temperature) { this->temperature_ = temperature; }
  void set_humidity_sensor(sensor::Sensor *humidity) { this->humidity_ = humidity; }
  void set_warm_mist_switch(switch_::Switch *warm_mist) { this->warm_mist_ = warm_mist; }
  void set_uv_switch(switch_::Switch *uv) { this->uv_ = uv; }
  void set_ionizer_switch(switch_::Switch *ionizer) { this->ionizer_ = ionizer; }
  void set_lock_switch(switch_::Switch *lock) { this->lock_ = lock; }
  void set_mute_switch(switch_::Switch *mute) { this->mute_ = mute; }
  void set_water_binary_sensor(binary_sensor::BinarySensor *water) { this->water_ = water; }
  void set_fan(fan::Fan *fan) { this->fan_ = fan; }
  void set_target_humidity_number(number::Number *target_humidity) { this->target_humidity_ = target_humidity; }
  void set_fan_speed_number(number::Number *fan_speed) { this->fan_speed_ = fan_speed; }
  void set_fan_preset_select(select::Select *fan_preset) { this->fan_preset_ = fan_preset; }
  void set_led_brightness_number(number::Number *led_brightness) { this->led_brightness_ = led_brightness; }
  void set_led_top_switch(switch_::Switch *led_top) { this->led_top_ = led_top_; }
  void set_led_bottom_switch(switch_::Switch *led_bottom) { this->led_bottom_ = led_bottom; }
  void set_led_preset_select(select::Select *led_preset) { this->led_preset_ = led_preset; }

 protected:
#ifdef USE_TIME
  esphome::time::RealTimeClock *time_{};
#endif

  sensor::Sensor *temperature_{};
  sensor::Sensor *humidity_{};

  switch_::Switch *warm_mist_{};
  switch_::Switch *uv_{};
  switch_::Switch *ionizer_{};
  switch_::Switch *lock_{};
  switch_::Switch *mute_{};

  binary_sensor::BinarySensor *water_{};

  fan::Fan *fan_{};

  number::Number *target_humidity_{};
  number::Number *fan_speed_{};
  select::Select *fan_preset_{};
  number::Number *led_brightness_{};
  switch_::Switch *led_top_{};
  switch_::Switch *led_bottom_{};
  select::Select *led_preset_{};

  void dump_config_(const char *TAG) const;

  template<class T, typename V> inline void publish_state_(T *var, V val) {
    if (var != nullptr && static_cast<V>(var->state) != val) {
      var->publish_state(val);
    }
  }

  void publish_fan_state_(const ehu_state_t &state);
  void write_fan_preset_(const std::string &preset) const;
  const std::string &get_fan_preset_(const ehu_state_t &state) const;
  const std::string &get_led_preset_(const ehu_state_t &state) const;
};

template<uint16_t cmd_v> class EHUCommandComponent : public Component, public Parented<EHUComponent> {
 public:
  EHUCommandComponent(EHUComponent *c) : Parented(c) {}

 protected:
  void write_byte_(uint8_t data) {
    if constexpr (cmd_v != 0) {
      this->parent_->api_->write_byte(cmd_v, data);
    }
  }
};

template<uint16_t cmd_v> class EHUSwitch : public switch_::Switch, public EHUCommandComponent<cmd_v> {
 public:
  EHUSwitch(EHUComponent *c) : EHUCommandComponent<cmd_v>(c) {}
  void write_state(bool state) override { this->write_byte_(state); }
};

template<uint16_t cmd_v, uint16_t state_flag_first_v, uint16_t state_flag_second_v>
class EHUDependedSwitch : public EHUSwitch<cmd_v> {
 public:
  EHUDependedSwitch(EHUComponent *c) : EHUSwitch<cmd_v>(c) {}
  void set_second(switch_::Switch *second) { this->second_ = second; }

 public:
  void write_state(bool state) override {
    uint8_t data = 0;
    if (state) {
      data |= state_flag_first_v;
    }
    if (this->second_ && this->second_->state) {
      data |= state_flag_second_v;
    }
    this->write_byte_(data);
  }

 protected:
  switch_::Switch *second_{};
};

class EHUFan : public Component, public fan::Fan, Parented<EHUComponent> {
 public:
  explicit EHUFan(EHUComponent *c) : Parented(c) {}
  fan::FanTraits get_traits() override;
  void control(const fan::FanCall &call) override;
};

template<uint16_t cmd_v> class EHUNumber : public number::Number, public EHUCommandComponent<cmd_v> {
 public:
  EHUNumber(EHUComponent *c) : EHUCommandComponent<cmd_v>(c) {}
  void control(float value) override { this->write_byte_(value); }
};

class EHUFanPreset : public select::Select, public Component, Parented<EHUComponent> {
 public:
  EHUFanPreset(EHUComponent *c) : Parented(c) {}
  void setup() override;
  void control(const std::string &value) override { this->parent_->write_fan_preset_(value); }
};

class EHULedPreset : public select::Select, public Component, Parented<EHUComponent> {
 public:
  EHULedPreset(EHUComponent *c) : Parented(c) {}
  void setup() override;
  void control(const std::string &value) override;
};

}  // namespace ehu
}  // namespace esphome
