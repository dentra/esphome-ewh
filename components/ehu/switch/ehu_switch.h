#pragma once

#include "esphome/core/component.h"

#include "../ehu_component.h"

namespace esphome {
namespace ehu {

class EHUPowerSwitch : public EHUComponent, public switch_::Switch {
 public:
  explicit EHUPowerSwitch(EHUApi *api) : EHUComponent(api) {}
  void dump_config() override;
  void on_state(const ehu_state_t &state) override {
    if (state.power != this->state) {
      this->publish_state(state.power);
    }
    EHUComponent::on_state(state);
  }
  void write_state(bool state) override { this->api_->set_power(state); }

 protected:
  bool get_power_state_() const override { return this->state; }
};

}  // namespace ehu
}  // namespace esphome
