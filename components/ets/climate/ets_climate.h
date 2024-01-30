#pragma once

#include "esphome/core/component.h"
#include "esphome/components/climate/climate.h"

#include "../ets_component.h"

namespace esphome {
namespace ets {

using namespace esphome::climate;

class ETSClimate : public ETSComponent, public Climate {
 public:
  explicit ETSClimate(ETSApi *api) : ETSComponent(api) {}

  void dump_config() override;
  ClimateTraits traits() override;
  void control(const ClimateCall &call) override;

  void on_state(const ets_state_t &state) override;
};

}  // namespace ets
}  // namespace esphome
