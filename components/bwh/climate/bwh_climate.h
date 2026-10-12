#pragma once

#include "esphome/core/component.h"
#include "esphome/components/climate/climate.h"
#include "esphome/components/sensor/sensor.h"

#include "../../rka_api/rka_heating.h"

#include "../bwh_component.h"

namespace esphome {
namespace bwh {

using namespace esphome::climate;

class BWHClimate : public BWHComponent, public Climate {
 public:
  explicit BWHClimate(BWHApi *api);

  void dump_config() override;
  ClimateTraits traits() override;
  void control(const ClimateCall &call) override;

  void on_state(const bwh_state_t &state) override;

  void set_idle_temp_drop(uint8_t idle_temp_drop) { this->heating_.set_idle_temp_drop(idle_temp_drop); }
  void set_power_sensor(sensor::Sensor *power) { this->power_ = power; }

 protected:
  rka_api::HeatingEstimator heating_;
  sensor::Sensor *power_{};

  void publish_power_(const bwh_state_t &state);
  bool update_heating_(const bwh_state_t &state);
  bwh_mode_t::Mode to_wh_mode_(ClimateMode mode, StringRef preset) const;
};

}  // namespace bwh
}  // namespace esphome
