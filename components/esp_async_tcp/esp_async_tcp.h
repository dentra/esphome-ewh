#pragma once

#include "esphome/core/log.h"

#include "etl/delegate.h"

#if defined(USE_ESP8266_FRAMEWORK_ARDUINO) || defined(USE_TESTS)
#include "ESPAsyncTCP.h"
#endif  // USE_ESP8266_FRAMEWORK_ARDUINO

namespace esphome {
namespace esp_async_tcp {

class ESPAsyncClientBase {
 public:
  using on_conn_type = etl::delegate<void()>;
  using on_data_type = etl::delegate<void(const void *data, size_t size)>;

  void on_connect(on_conn_type &&cb) { this->cb_connect_ = std::move(cb); }
  void on_disconnect(on_conn_type &&cb) { this->cb_disconnect_ = std::move(cb); }
  void on_data(on_data_type &&cb) { this->cb_data_ = std::move(cb); }

 protected:
  on_conn_type cb_connect_;
  on_conn_type cb_disconnect_;
  on_data_type cb_data_;
};

#if defined(USE_ESP8266_FRAMEWORK_ARDUINO) || defined(USE_TESTS)
class ESPAsyncClient8266 : public ESPAsyncClientBase {
 public:
  ESPAsyncClient8266();
  ~ESPAsyncClient8266() { this->close(); }
  bool connect(const char *host, uint16_t port) { return this->client_.connect(host, port); }
  void close() { this->client_.close(); }
  bool is_connected() { return this->client_.connected(); }
  size_t send(const char *data, size_t size) {
    size_t res = this->client_.add(data, size, 0);
    return this->client_.send() ? res : 0;
  }
  void loop() {}
#ifdef USE_TESTS
  AsyncClient *client() { return &this->client_; }
#endif
 protected:
  AsyncClient client_;
};

template<size_t recv_buf_size = 128> using ESPAsyncClient = ESPAsyncClient8266;
#endif  // USE_ESP8266_FRAMEWORK_ARDUINO

#if defined(USE_ESP32) && !defined(USE_TESTS)
class ESPAsyncClient32 : public ESPAsyncClientBase {
 public:
  bool connect(const char *host, uint16_t port);
  void close();
  size_t send(const char *data, size_t size);
  bool is_connected() { return this->sock_ >= 0; }

 protected:
  int sock_{-1};

  void loop_(uint8_t *buf, size_t recv_buf_size);
};

template<size_t recv_buf_size = 128> class ESPAsyncClient : public ESPAsyncClient32 {
 public:
  ~ESPAsyncClient() { this->close(); }
  void loop() {
    if (this->sock_ < 0) {
      return;
    }
    uint8_t buf[recv_buf_size];
    this->loop_(buf, recv_buf_size);
  }
};
#endif  // USE_ESP32

}  // namespace esp_async_tcp
}  // namespace esphome
