#pragma once

#include "esphome/core/component.h"
#include "esphome/components/climate/climate.h"

// #ifdef USE_API
// #include "esphome/components/api/custom_api_device.h"
// #endif

#include "../ewh_component.h"

namespace esphome {
namespace ewh {

using namespace esphome::climate;

class EWHClimate : public EWHComponent, public Climate {
 public:
  explicit EWHClimate(ElectroluxWaterHeater *ewh) : EWHComponent(ewh) {}

  void dump_config() override;
  ClimateTraits traits() override;
  void control(const ClimateCall &call) override;

  // void read(const ewh_state_t &state) override;
  void read(const ewh_status_t &state) override;

 protected:
  ewh_mode_t::Mode to_wh_mode_(ClimateMode mode, const std::string &preset) const;
};  // namespace ewh

}  // namespace ewh
}  // namespace esphome
