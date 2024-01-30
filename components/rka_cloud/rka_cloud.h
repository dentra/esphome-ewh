#pragma once

#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/uart/uart_component.h"
#include "esphome/components/switch/switch.h"

#include "../esp_async_tcp/esp_async_tcp.h"

#include "../rka_api/rka_api.h"
#include "../rka_api/rka_vport.h"

namespace esphome {
namespace rka_cloud {

/// High-Flying HF-LPT220 Emulation
class RKACloud : public switch_::Switch, public Component, public rka_api::RKAVPortListener {
  using TCPClient = esp_async_tcp::ESPAsyncClient<64>;

 public:
  explicit RKACloud(rka_api::RKAVPort *vport) : vport_(vport) { vport->add_listener(this); }
  virtual ~RKACloud() { this->disconnect(); }

  float get_setup_priority() const override { return setup_priority::AFTER_WIFI; }
  void setup() override;
  void dump_config() override;

  void loop() override {
    if (this->tcp_) {
      this->tcp_->loop();
    }
  }

  void set_host(const char *host) { this->host_ = host; }
  void set_port(const uint16_t port) { this->port_ = port; }

  /// MAC-address in direct form e.g. 11:22:33:44:55:66 -> {0x11, 0x22, 0x33, 0x44, 0x55, 0x66}
  void set_mac(const uint8_t mac[6]) { std::memcpy(this->mac_, mac, sizeof(this->mac_)); }

  /// UID split in decimal parts and set as hex e.g 263704 -> {0, 26, 37, 4} == {0, 0x1a, 0x25, 4}
  void set_uid(const uint8_t uid[4]) { std::memcpy(this->uid_.data, uid, sizeof(this->uid_.data)); }

  void set_cloud_pair(switch_::Switch *cloud_pair) { this->cloud_pair_ = cloud_pair; }

  void on_frame(const rka_api::rka_any_frame_t &frame, size_t size) override;

  void connect();
  void disconnect();

  bool is_connected() { return this->tcp_ && this->tcp_->is_connected(); }

  const uint8_t *get_mac() const { return this->mac_; }

 protected:
  uint8_t mac_[6]{};
  union {
    uint8_t data[4];
    uint32_t value;
  } uid_{};
  rka_api::RKAVPort *vport_;

  const char *host_{};
  uint16_t port_{};

  TCPClient *tcp_{};

  switch_::Switch *cloud_pair_{};

  using io_t = rka_api::UartFrameIO<sizeof(rka_api::rka_any_frame_t)>;
  io_t io_;

  size_t process_at_(const char *cmd, size_t len);
  void process_aa_(const io_t::rx_frame_t *req, size_t size);
  bool is_mac_set_() const;

  void write_state(bool state) override {
    this->disconnect();
    if (state) {
      this->connect();
    }
  }

  void on_data_(const void *data, size_t size);
  void on_connect_();
  void on_disconnect_();
};

}  // namespace rka_cloud
}  // namespace esphome
