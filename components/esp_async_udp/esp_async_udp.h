#pragma once

#include <functional>

#ifdef USE_ESP8266
#include <WiFiUdp.h>
#endif

namespace esphome {
namespace esp_async_udp {

class ESPAsyncUDPBase {
 public:
  using on_packet_type = std::function<void(const uint8_t *data, size_t size, uint32_t addr, uint16_t port)>;
  void on_packet(on_packet_type &&cb) { this->cb_packet_ = std::move(cb); }

 protected:
  on_packet_type cb_packet_;
};

#ifdef USE_ESP8266
class ESPAsyncUDP8266 : public ESPAsyncUDPBase {
 public:
  bool listen_multicast(const char *group_addr, uint16_t port);
  void close();
  ssize_t sendto(const uint8_t *data, size_t size, uint32_t addr, uint16_t port);

 protected:
  WiFiUDP udp_;
  IPAddress group_;
  void loop_(uint8_t *buf, size_t recv_buf_size);
};

template<size_t recv_buf_size = 16> class ESPAsyncUDP : public ESPAsyncUDP8266 {
 public:
  ~ESPAsyncUDP() { this->close(); }
  void loop() {
    uint8_t buf[recv_buf_size]{};
    this->loop_(buf, recv_buf_size);
  }
};
#endif

#ifdef USE_ESP32
class ESPAsyncUDP32 : public ESPAsyncUDPBase {
 public:
  bool listen_multicast(const char *group_addr, uint16_t port);
  void close();
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
#endif

}  // namespace esp_async_udp
}  // namespace esphome
