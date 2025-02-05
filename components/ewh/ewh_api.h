#pragma once

#include "vport/ewh_vport.h"
#include "ewh_data.h"

namespace esphome {
namespace ewh {

class EWHListener : public rka_api::RKAListener<ewh_state_t> {
 public:
  void on_frame(const rka_api::rka_any_frame_t &frame, size_t size) override {
    if (frame.type == rka_api::PACKET_CMD_STATE) {
      this->on_state(frame.data, size);
    } else {
      rka_api::RKAListener<ewh_state_t>::on_frame(frame, size);
    }
  }
};

using EWHApiBase = rka_api::RKAApi<EWHVPort>;

class EWHApi : public EWHApiBase {
 public:
  explicit EWHApi(EWHVPort *vport) : EWHApiBase(vport) {}

  void set_mode(ewh_mode_t::Mode mode, uint8_t target_temperature) const;
  void set_clock(uint8_t hours, uint8_t minutes) const;
  void set_timer(uint8_t hours, uint8_t minutes, uint8_t temperature,
                 ewh_mode_t::Mode mode = ewh_mode_t::MODE_700W) const;
  void set_bst(bool value) const;

  void set_clock(const ewh_clock_t &clock) const;
  void set_timer(const ewh_timer_t &timer) const;
  void set_mode(const ewh_mode_t &mode) const;
  void set_bst(const ewh_bst_t &bst) const;

 protected:
};

}  // namespace ewh
}  // namespace esphome
