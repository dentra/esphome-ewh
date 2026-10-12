#pragma once

#include "esphome/core/component.h"
#include "esphome/components/climate/climate.h"
#include "esphome/components/sensor/sensor.h"

#include "../../rka_api/rka_heating.h"

#include "../ewh_component.h"

namespace esphome {
namespace ewh {

using namespace esphome::climate;

class EWHClimate : public EWHComponent, public Climate {
 public:
  explicit EWHClimate(EWHApi *api);

  void dump_config() override;
  ClimateTraits traits() override;
  void control(const ClimateCall &call) override;

  void on_state(const ewh_state_t &state) override;

  void set_idle_temp_drop(uint8_t idle_temp_drop) { this->heating_.set_idle_temp_drop(idle_temp_drop); }
  void set_power_sensor(sensor::Sensor *power) { this->power_ = power; }

 protected:
  rka_api::HeatingEstimator heating_;
  sensor::Sensor *power_{};

  void publish_power_(const ewh_state_t &state);
  bool update_heating_(const ewh_state_t &state);
  ewh_mode_t::Mode to_wh_mode_(ClimateMode mode, StringRef preset) const;
};

}  // namespace ewh
}  // namespace esphome
