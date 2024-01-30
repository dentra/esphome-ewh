#pragma once

#include "esphome/core/component.h"
#include "esphome/components/climate/climate.h"

#include "../bwh_component.h"

namespace esphome {
namespace bwh {

using namespace esphome::climate;

class BWHClimate : public BWHComponent, public Climate {
 public:
  explicit BWHClimate(BWHApi *api) : BWHComponent(api) {}

  void dump_config() override;
  ClimateTraits traits() override;
  void control(const ClimateCall &call) override;

  void on_state(const bwh_state_t &state) override;

 protected:
  bwh_mode_t::Mode to_wh_mode_(ClimateMode mode, const std::string &preset) const;
};

}  // namespace bwh
}  // namespace esphome
