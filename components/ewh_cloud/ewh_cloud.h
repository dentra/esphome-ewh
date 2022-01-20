#pragma once
#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/uart/uart_component.h"
#include "esphome/components/text_sensor/text_sensor.h"

#include <ESPAsyncTCP.h>
#include <ESPAsyncUDP.h>

#include "../ewh/ewh.h"

namespace esphome {
namespace ewh {

class EWHCloud : public Component, public EWHListener {
 public:
  float get_setup_priority() const override { return setup_priority::AFTER_WIFI; }
  void setup() override;

  void set_host(const char *host) { this->host_ = host; }
  void set_port(const uint16_t port) { this->port_ = port; }

  void set_mac(const uint8_t mac[6]) { std::memcpy(this->mac_, mac, sizeof(this->mac_)); }
  void set_key(const uint8_t uid[3]) { std::memcpy(this->uid_.data, uid, sizeof(this->uid_.data)); }

  void on_tcp_data(const uint8_t *data, uint32_t size);

  void read(const ewh_status_t &status) override { this->read(PACKET_STATUS, &status, sizeof(status)); }
  void read(const ewh_state_t &state) override { this->read(PACKET_RSP_STATE, &state, sizeof(state)); }
  void read(const ewh_dev_type_t &dev_type) { this->read(PACKET_RSP_DEV_TYPE, &dev_type, sizeof(dev_type)); }
  void read(const ewh_result_t &result) override { this->read(PACKET_RSP_SAVE_DATA, &result, sizeof(result)); }
  void read(const ewh_data_t &data) override { this->read(PACKET_RSP_LOAD_DATA, &data, sizeof(data)); }
  void read(const ewh_error_t &error) override { this->read(PACKET_ERROR, &error, sizeof(error)); }
  void read(const PacketType type, const void *data, uint32_t size) override;

  void connect();
  void reconnect() {
    this->disconnect();
    this->connect();
  }
  void disconnect();
  bool is_connected() { return this->tcp_ && this->tcp_->connected(); }

  void pair(bool enable);
  bool is_pairing() { return this->udp_; }

 protected:
  uint8_t mac_[6]{};
  union {
    uint8_t data[3];
    uint32_t value;
  } uid_{};
  ElectroluxWaterHeater *ewh_;

  AsyncClient *tcp_{};
  std::string host_;
  uint16_t port_{};

  AsyncUDP *udp_{};

  void on_at_(const char *cmd, uint32_t size);
  void on_aa_(const uint8_t *data, uint32_t size);
};

}  // namespace ewh
}  // namespace esphome
