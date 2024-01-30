#pragma once

#include "esphome/core/component.h"
#include "esphome/components/switch/switch.h"

#include "../esp_async_udp/esp_async_udp.h"

#include "rka_cloud.h"

namespace esphome {
namespace rka_cloud {

class RKACloudPair : public switch_::Switch, public Component {
  using UDPServer = esp_async_udp::ESPAsyncUDP<16>;

 public:
  explicit RKACloudPair(RKACloud *cloud) : cloud_(cloud) {}
  virtual ~RKACloudPair() { delete this->udp_; }
  void loop() override {
    if (this->udp_) {
      this->udp_->loop();
    }
  }
  void write_state(bool state) override { this->pair(state); }
  bool is_pairing() { return this->udp_; }
  void pair(bool enable) {
    this->pair_(enable);
    this->publish_state(this->is_pairing());
  }

 protected:
  RKACloud *cloud_;
  UDPServer *udp_{};
  void pair_(bool enable);
  void on_packet_(const uint8_t *data, size_t size, uint32_t addr, uint16_t port);
};

}  // namespace rka_cloud
}  // namespace esphome
