#pragma once

#include "esphome/core/component.h"
#include "esphome/components/climate/climate.h"
#include "esphome/components/switch/switch.h"
#include "esphome/components/text_sensor/text_sensor.h"

#include "esphome/components/time/real_time_clock.h"
// #ifdef USE_API
#include "esphome/components/api/custom_api_device.h"
// #endif

#include "../ewh_component.h"

namespace esphome {
namespace ewh {

using namespace esphome::climate;

class EWHClimate : public EWHComponent, public Climate {
 public:
  explicit EWHClimate(UARTComponent *parent) : EWHComponent(parent) {}

  ClimateTraits traits() override;
  void control(const ClimateCall &call) override;

  void read(const ewh_state_t &state) override;

};  // namespace ewh

}  // namespace ewh
}  // namespace esphome
