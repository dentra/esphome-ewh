#pragma once

#include "esphome/components/uart/uart.h"
#include "esphome/components/uart/uart_component.h"
#include "esphome/components/time/real_time_clock.h"
#include "ewh_data.h"

namespace esphome {
namespace ewh {

using namespace esphome::uart;

class EWHListener {
 public:
  // read PACKET_STATUS.
  virtual void read(const ewh_status_t &status) {}
  // read PACKET_REQ_STATE.
  virtual void read(const ewh_state_t &state) { this->read(*static_cast<const ewh_status_t *>(&state)); }
  // read PACKET_RSP_DEV_TYPE.
  virtual void read(const ewh_dev_type_t &dev_type) {}
  // read PACKET_REQ_SAVE_DATA.
  virtual void read(const ewh_result_t &result) {}
  // read PACKET_RSP_LOAD_DATA.
  virtual void read(const ewh_data_t &data) {}
  // read PACKET_ERROR
  virtual void read(const ewh_error_t &error) {}
  // read unknow data packet
  virtual void read(const PacketType type, const void *data, uint32_t size) {}
};

class ElectroluxWaterHeater : public Component, private EWHListener {
 public:
  explicit ElectroluxWaterHeater(UARTComponent *uart) : uart_(uart) { this->add_listener(this); }

  void setup() override { this->request_dev_type(); }
  void loop() override { this->read_data_(); }

  void set_time_id(time::RealTimeClock *time) { this->time_ = time; }
  void add_listener(EWHListener *listener) { this->listeners_.push_back(listener); }

  void exec(uint8_t cmd, const uint8_t *data, uint32_t size);
  void exec(uint8_t cmd, const std::vector<uint8_t> &data = {});
  void exec(const std::string &data);

  void request_state();
  void request_dev_type();
  void set_mode(ewh_mode_t::Mode mode, uint8_t target_temperature);
  void set_clock(uint8_t hours, uint8_t minutes);
  void set_timer(uint8_t hours, uint8_t minutes, uint8_t temperature, ewh_mode_t::Mode mode = ewh_mode_t::MODE_700W);
  void set_bst(bool value);

  void set_clock(const ewh_clock_t &clock);
  void set_timer(const ewh_timer_t &timer);
  void set_mode(const ewh_mode_t &mode);
  void set_bst(const ewh_bst_t &bst);

  // write raw data to uart and flush
  void write_raw(const uint8_t *data, const uint32_t size) {
    this->uart_->write_array(data, size);
    this->uart_->flush();
  }

 private:
  void read(const ewh_state_t &state) override;
  void read(const ewh_error_t &error) override;
  void read(const ewh_dev_type_t &dev_type) override;
  void read(const ewh_result_t &result) override;
  void read(const PacketType type, const void *data, uint32_t size) override;

 protected:
  uart::UARTComponent *uart_;
  time::RealTimeClock *time_{};
  std::vector<EWHListener *> listeners_;

  uint8_t buf_[30];
  uint32_t buf_size_ = 0;
  int32_t await_size_ = 0;

  void read_data_();
  void read_frame_(const uint8_t *data, uint32_t size, uint8_t crc);
  void read_packet_(const PacketType packet_rsp_type, const void *data, uint32_t size);
  bool read_packet_check_(const uint32_t actual, const uint32_t expected) const;

  void send_packet_(const PacketType packet_req_type, const uint8_t *data, const uint32_t size);

  void send_control_state_(uint8_t operation, const void *data, int size);
};

}  // namespace ewh
}  // namespace esphome
