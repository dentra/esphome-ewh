#pragma once

#include "esphome/core/component.h"
#include "esphome/components/switch/switch.h"

#include "ewh.h"

namespace esphome {
namespace ewh {

class EWHComponent : public Component, public EWHListener {
 public:
  // BST (Bacteria Stop Technology) Switch.
  class BSTSwitch : public switch_::Switch {
   public:
    explicit BSTSwitch(ElectroluxWaterHeater *ewh) : ewh_(ewh) {}
    void write_state(bool state) override { this->ewh_->set_bst(state); }

   protected:
    ElectroluxWaterHeater *ewh_;
  };

  explicit EWHComponent(ElectroluxWaterHeater *ewh) : ewh_(ewh) { this->ewh_->add_listener(this); }

  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::AFTER_WIFI; }

  void set_bst(switch_::Switch *bst) { this->bst_ = bst; }

  // void set_idle_temp_drop(uint8_t idle_temp_drop) { this->idle_temp_drop_ = idle_temp_drop; }

  void read(const ewh_status_t &state) override {
    if (this->bst_ != nullptr) {
      this->bst_->publish_state(state.bst.state != ewh_bst_t::STATE_OFF);
    }
  }
  void read(const ewh_dev_type_t &dev_type) override { this->dev_type_ = dev_type; }

 protected:
  ElectroluxWaterHeater *ewh_;
  switch_::Switch *bst_{};
  ewh_dev_type_t dev_type_{};

  // uint8_t idle_temp_drop_ = {};
  // uint8_t prev_temp_ = {};
  // bool is_idle_(const ewh_state_t &state);
};  // namespace ewh

}  // namespace ewh
}  // namespace esphome
