#include "esphome/core/log.h"
#include "esphome/core/helpers.h"

#include "esp_async_tcp.h"

namespace esphome {
namespace esp_async_tcp {
static const char *TAG = "esp_async_tcp";
#if defined(USE_ESP8266_FRAMEWORK_ARDUINO) || defined(USE_TESTS)
ESPAsyncClient8266::ESPAsyncClient8266() {
  this->client_.onConnect(
      [](void *arg, AsyncClient *) {
        auto self = static_cast<ESPAsyncClient8266 *>(arg);
        if (self->cb_connect_) {
          self->cb_connect_();
        }
      },
      this);
  this->client_.onDisconnect(
      [](void *arg, AsyncClient *) {
        auto self = static_cast<ESPAsyncClient8266 *>(arg);
        if (self->cb_disconnect_) {
          self->cb_disconnect_();
        }
      },
      this);
  this->client_.onData(
      [](void *arg, AsyncClient *, void *data, size_t size) {
        ESP_LOGV(TAG, "Received data: %s", format_hex_pretty(static_cast<uint8_t *>(data), size).c_str());
        auto self = static_cast<ESPAsyncClient8266 *>(arg);
        if (self->cb_data_) {
          self->cb_data_(data, size);
        }
      },
      this);
}
#endif
#if defined(USE_ESP32) && !defined(USE_TESTS)
#include "esphome/components/network/util.h"
#include "esphome/components/socket/headers.h"
#include "lwip/err.h"
#include "lwip/api.h"

bool ESPAsyncClient32::connect(const char *host, uint16_t port) {
  // TODO actualise
  // if (!network::is_connected()) {
  //   return false;
  // }

  this->close();

  ip_addr_t ipaddr{};
  auto err = netconn_gethostbyname_addrtype(host, &ipaddr, NETCONN_DNS_IPV4);
  if (err != ERR_OK) {
    return false;
  }

  this->sock_ = lwip_socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
  if (this->sock_ < 0) {
    ESP_LOGW(TAG, "socket failed: errno %d, %s", errno, strerror(errno));
    return false;
  }

  // Marking the socket as non-blocking
  // fcntl(this->sock_, F_SETFL, fcntl(this->sock_, F_GETFL) | O_NONBLOCK);

  sockaddr_in saddr{};
  saddr.sin_family = AF_INET;
  saddr.sin_port = htons(port);
#if LWIP_IPV6
  saddr.sin_addr.s_addr = ipaddr.u_addr.ip4.addr;
#else
  saddr.sin_addr.s_addr = ipaddr.addr;
  // inet_addr_to_ip4addr(ip_2_ip4(ipaddr), &saddr.sin_addr);
#endif

  auto res = lwip_connect(this->sock_, (sockaddr *) &saddr, sizeof(saddr));
  if (res != 0) {
    ESP_LOGW(TAG, "connect failed: errno %d, %s", errno, strerror(errno));
    lwip_close(this->sock_);
    this->sock_ = -1;
    return false;
  }

  if (this->cb_connect_) {
    this->cb_connect_();
  } else {
    ESP_LOGD(TAG, "no listener configured");
  }

  return true;
}
void ESPAsyncClient32::close() {
  if (this->sock_ < 0) {
    return;
  }
  lwip_close(this->sock_);
  this->sock_ = -1;
  if (this->cb_disconnect_) {
    this->cb_disconnect_();
  }
}
size_t ESPAsyncClient32::send(const char *data, size_t size) {
  if (this->sock_ < 0) {
    return 0;
  }
  return lwip_send(this->sock_, data, size, 0);
}

void ESPAsyncClient32::loop_(uint8_t *buf, size_t recv_buf_size) {
  ssize_t len = lwip_recv(this->sock_, buf, recv_buf_size, MSG_DONTWAIT);
  if (len < 0) {
    return;
  }
  ESP_LOGV(TAG, "Received data: %s", format_hex_pretty(buf, len).c_str());
  if (this->cb_data_) {
    this->cb_data_(buf, len);
  }
}

#endif  // USE_ESP32
}  // namespace esp_async_tcp
}  // namespace esphome
