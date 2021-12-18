#pragma once

#include "esphome/core/component.h"
#include "esphome/components/switch/switch.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/time/real_time_clock.h"

#include "ewh.h"

namespace esphome {
namespace ewh {

class EWHComponent : public ElectroluxWaterHeater, public Component {
 public:
  // BST (Bacteria Stop Technology) Switch.
  class BSTSwitch : public switch_::Switch {
   public:
    explicit BSTSwitch(EWHComponent *parent) : parent_(parent) {}
    void write_state(bool state) override { this->parent_->send_set_bst(state); }

   protected:
    EWHComponent *parent_;
  };

  explicit EWHComponent(UARTComponent *parent) : ElectroluxWaterHeater(parent) {}

  void loop() override { this->read_data_(); }

  void set_time(time::RealTimeClock *time) { this->time_ = time; }
  void set_bst(switch_::Switch *bst) { this->bst_ = bst; }
  void set_clock(text_sensor::TextSensor *clock) { this->clock_ = clock; }
  void set_timer(text_sensor::TextSensor *timer) { this->timer_ = timer; }

  void read(const ewh_state_t &state) override;
  void sync_clock();

  void exec(uint8_t cmd, const std::vector<uint8_t> &data = {});
  void exec(const std::string &data);
  void timer(const int hours, const int minutes, const int temperature);

 protected:
  time::RealTimeClock *time_{};
  switch_::Switch *bst_{};
  text_sensor::TextSensor *clock_{};
  text_sensor::TextSensor *timer_{};

};  // namespace ewh

}  // namespace ewh
}  // namespace esphome
