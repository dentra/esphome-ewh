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

  void set_power(bool power);
  void set_preset(uint8_t preset);

 protected:
  void write_byte(uint8_t cmd, uint8_t data) {
    struct {
      uint8_t data;
    } PACKED op{.data = data};
    this->write(cmd, op);
  }
};

}  // namespace ehu
}  // namespace esphome
