#pragma once

#include "esphome/core/component.h"
#include "esphome/components/switch/switch.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/time/real_time_clock.h"

#include "ewh.h"
#include "ewh_cloud.h"

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

  explicit EWHComponent(UARTComponent *parent) : ElectroluxWaterHeater(parent) { this->cloud = new EWHCloud(this); }
  ~EWHComponent() { delete cloud; }

  float get_setup_priority() const override { return setup_priority::AFTER_WIFI; }

  void loop() override { this->read_data_(); }

  void set_time(time::RealTimeClock *time) { this->time_ = time; }
  void set_bst(switch_::Switch *bst) { this->bst_ = bst; }
  void set_clock(text_sensor::TextSensor *clock) { this->clock_ = clock; }
  void set_timer(text_sensor::TextSensor *timer) { this->timer_ = timer; }
  void set_debug(text_sensor::TextSensor *debug) { this->debug_ = debug; }
  text_sensor::TextSensor *get_debug() const { return this->debug_; }

  void read(const ewh_state_t &state) override;
  void read(const PacketType type, const uint8_t *data, uint32_t size) override;
  bool read(const ewh_unknown81 &unk) override;
  bool read(const ewh_unknown86 &unk) override;
  bool read(const ewh_unknown87 &unk) override;

  void sync_clock();

  void exec(uint8_t cmd, const std::vector<uint8_t> &data = {});
  void exec(const std::string &data);
  void timer(const int hours, const int minutes, const int temperature);

  EWHCloud *cloud;

 protected:
  time::RealTimeClock *time_{};
  switch_::Switch *bst_{};
  text_sensor::TextSensor *clock_{};
  text_sensor::TextSensor *timer_{};
  text_sensor::TextSensor *debug_{};

  void read_frame_(const uint8_t *data, uint32_t size, uint8_t crc) override {
    cloud->on_device_frame(data, size, crc);
    ElectroluxWaterHeater::read_frame_(data, size, crc);
  }
};  // namespace ewh

}  // namespace ewh
}  // namespace esphome
