#pragma once

#include "esphome/core/component.h"
#include "esphome/components/climate/climate.h"

#include "../ewh_component.h"

namespace esphome {
namespace ewh {

using namespace esphome::climate;

class EWHClimate : public EWHComponent, public Climate {
 public:
  explicit EWHClimate(EWHApi *api) : EWHComponent(api) {}

  void dump_config() override;
  ClimateTraits traits() override;
  void control(const ClimateCall &call) override;

  void on_state(const ewh_state_t &state) override;

 protected:
  ewh_mode_t::Mode to_wh_mode_(ClimateMode mode, const std::string &preset) const;
};

}  // namespace ewh
}  // namespace esphome
