#pragma once

#include "esphome/core/component.h"
#include "esphome/components/fan/fan.h"

#include "../ehu_component.h"

namespace esphome {
namespace ehu {

class EHUFan : public EHUComponent, public fan::Fan {
 public:
  explicit EHUFan(EHUApi *api) : EHUComponent(api) {}

  void dump_config() override;
  fan::FanTraits get_traits() override;
  void control(const fan::FanCall &call) override;

  void on_state(const ehu_state_t &state) override;
};

}  // namespace ehu
}  // namespace esphome
