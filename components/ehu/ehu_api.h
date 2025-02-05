#pragma once

#include "../rka_api/rka_api.h"
#include "vport/ehu_vport.h"
#include "ehu_data.h"

namespace esphome {
namespace ehu {

class EHUListener : public rka_api::RKAListener<ehu_state_t> {
 public:
  void on_frame(const rka_api::rka_any_frame_t &frame, size_t size) override {
    if (frame.type == rka_api::PACKET_CMD_STATE) {
      this->on_state(frame.data, size);
    } else {
      rka_api::RKAListener<ehu_state_t>::on_frame(frame, size);
    }
  }
};

using EHUApiBase = rka_api::RKAApi<EHUVPort>;

class EHUApi : public EHUApiBase {
 public:
  explicit EHUApi(EHUVPort *vport) : EHUApiBase(vport) {}

  void set_power(bool power) const;
  void set_preset(uint8_t preset) const;
  void set_speed(uint8_t speed) const;
  void set_humidity(uint8_t humidity) const;

  void set_ionizer(bool lock) const;
  void set_lock(bool lock) const;
  void set_mute(bool mute) const;

  void set_clock(uint8_t hours, uint8_t minutes) const;
  void set_led_preset(uint8_t led_preset) const;
};

}  // namespace ehu
}  // namespace esphome
