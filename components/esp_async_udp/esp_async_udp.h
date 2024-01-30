#pragma once

#include "etl/delegate.h"

#if defined(USE_ESP8266_FRAMEWORK_ARDUINO) || defined(USE_TESTS)
#include "ESPAsyncUDP.h"
#endif  // USE_ESP8266_FRAMEWORK_ARDUINO

namespace esphome {
namespace esp_async_udp {

class ESPAsyncUDPBase {
 public:
  using on_packet_type = etl::delegate<void(const uint8_t *data, size_t size, uint32_t addr, uint16_t port)>;
  void on_packet(on_packet_type &&cb) { this->cb_packet_ = std::move(cb); }

 protected:
  on_packet_type cb_packet_;
};

#if defined(USE_ESP8266_FRAMEWORK_ARDUINO) || defined(USE_TESTS)
class ESPAsyncUDP8266 : public ESPAsyncUDPBase {
 public:
  ESPAsyncUDP8266();
  ~ESPAsyncUDP8266() { this->close(); }
  bool listen_multicast(const char *group_addr, uint16_t port);
  void close() { this->udp_.close(); }
  bool is_connected() { return this->udp_.connected(); }
  ssize_t sendto(const uint8_t *data, size_t size, uint32_t addr, uint16_t port);
  void loop() {}
#ifdef USE_TESTS
  AsyncUDP *client() { return &this->udp_; }
#endif
 protected:
  AsyncUDP udp_;
};

template<size_t recv_buf_size = 16> using ESPAsyncUDP = ESPAsyncUDP8266;
#endif

#if defined(USE_ESP32) && !defined(USE_TESTS)
class ESPAsyncUDP32 : public ESPAsyncUDPBase {
 public:
  bool listen_multicast(const char *group_addr, uint16_t port);
  void close();
  bool is_connected() { return this->sock_ >= 0; }
  ssize_t sendto(const uint8_t *data, size_t size, uint32_t addr, uint16_t port);

 protected:
  int sock_{-1};
  void loop_(uint8_t *buf, size_t recv_buf_size);
};

template<size_t recv_buf_size = 16> class ESPAsyncUDP : public ESPAsyncUDP32 {
 public:
  ~ESPAsyncUDP() { this->close(); }
  void loop() {
    if (this->sock_ < 0) {
      return;
    }
    uint8_t buf[recv_buf_size]{};
    this->loop_(buf, recv_buf_size);
  }
};
#endif  // USE_ESP32

}  // namespace esp_async_udp
}  // namespace esphome

// #endif
