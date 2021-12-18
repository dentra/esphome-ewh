#pragma once

#include "esphome/components/uart/uart.h"
#include "esphome/components/uart/uart_component.h"

#include "ewh_data.h"

namespace esphome {
namespace ewh {

using namespace esphome::uart;

class ElectroluxWaterHeater : public UARTDevice {
 public:
  explicit ElectroluxWaterHeater(UARTComponent *parent) : UARTDevice(parent) {}

  virtual void read(const ewh_state_t &state){};
  virtual void read(const ewh_error_t &error);

 protected:
  uint8_t buf_[30];
  int buf_size_ = 0;
  int await_size_ = 0;

  uint8_t crc8_(uint8_t crc, const uint8_t *data, size_t size) const {
    while (size--) {
      crc += *data++;
    }
    return crc;
  }

  uint8_t crc8_(const uint8_t frame_header, const uint8_t frame_size, const uint8_t *data, size_t size) const {
    return this->crc8_(frame_header + frame_size, data, size);
  }

  uint8_t crc8_(const uint8_t frame_header, const uint8_t frame_size, const uint8_t frame_type, const uint8_t *data,
                size_t size) const {
    return this->crc8_(frame_header + frame_size + frame_type, data, size);
  }

  void read_data_();
  void read_frame_(const uint8_t *data, uint32_t size, uint8_t crc);
  void read_packet_(const PacketType packet_rsp_type, const void *data, uint32_t size);
  bool read_packet_check_(const uint32_t actual, const uint32_t expected) const;

  void send_packet_(const PacketType packet_req_type, const uint8_t *data, const uint32_t size);

  void send_request_state_();
  void send_control_state_(uint8_t operation, const void *data, int size);
  void send_set_bst(bool value);
  void send_set_clock(uint8_t hours, uint8_t minutes);
  void send_set_timer(uint8_t hours, uint8_t minutes, uint8_t mode, uint8_t temperature);
};

}  // namespace ewh
}  // namespace esphome
