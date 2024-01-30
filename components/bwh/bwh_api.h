#pragma once

#include "../rka_api/rka_api.h"
#include "vport/bwh_vport.h"
#include "bwh_data.h"

namespace esphome {
namespace bwh {

class BWHListener : public rka_api::RKAListener<bwh_state_t> {
 public:
  void on_frame(const rka_api::rka_any_frame_t &frame, size_t size) override {
    if (frame.type == rka_api::PACKET_CMD_STATE) {
      this->on_state(frame.data, size);
    } else {
      rka_api::RKAListener<bwh_state_t>::on_frame(frame, size);
    }
  }
};

using BWHApiBase = rka_api::RKAApi<BWHVPort>;

class BWHApi : public BWHApiBase {
 public:
  explicit BWHApi(BWHVPort *vport) : BWHApiBase(vport) {}

  void set_mode(bwh_mode_t::Mode mode, uint8_t target_temperature);
  void set_mode(const bwh_mode_t &mode);

 protected:
};

}  // namespace bwh
}  // namespace esphome
