#include "esphome/core/log.h"
#include "esphome/core/helpers.h"

#include "esp_async_udp.h"

#ifdef USE_ESP8266
#include <lwip/igmp.h>
#endif

#ifdef USE_ESP32
#include "esphome/components/socket/headers.h"
#endif

namespace esphome {
namespace esp_async_udp {
static const char *TAG = "esp_async_udp";

#ifdef USE_ESP8266
bool ESPAsyncUDP8266::listen_multicast(const char *group_addr, uint16_t port) {
  this->close();

  IPAddress group;
  if (!group.fromString(group_addr)) {
    ESP_LOGW(TAG, "Invalid multicast group %s", group_addr);
    return false;
  }

  // group may be joined even if listen fails, so close() must leave it
  this->group_ = group;
  // listen on all interfaces
  if (!this->udp_.beginMulticast(IPAddress(0, 0, 0, 0), group, port)) {
    ESP_LOGW(TAG, "beginMulticast failed");
    this->close();
    return false;
  }

  return true;
}

void ESPAsyncUDP8266::close() {
  this->udp_.stop();
  // WiFiUDP::stop() does not leave multicast group
  if (this->group_.isSet()) {
    igmp_leavegroup(IPAddress(0, 0, 0, 0), this->group_);
    this->group_ = IPAddress();
  }
}

ssize_t ESPAsyncUDP8266::sendto(const uint8_t *data, size_t size, uint32_t addr, uint16_t port) {
  if (!this->udp_.beginPacket(IPAddress(addr), port)) {
    return -1;
  }
  auto len = this->udp_.write(data, size);
  if (!this->udp_.endPacket()) {
    return -1;
  }
  return len;
}

void ESPAsyncUDP8266::loop_(uint8_t *buf, size_t recv_buf_size) {
  // one packet per loop, same as esp32
  if (this->udp_.parsePacket() <= 0) {
    return;
  }
  auto len = this->udp_.read(buf, recv_buf_size);
  if (len <= 0) {
    return;
  }

  ESP_LOGV(TAG, "Received packet from %s:%u: %s", this->udp_.remoteIP().toString().c_str(), this->udp_.remotePort(),
           format_hex_pretty(buf, len).c_str());

  if (this->cb_packet_) {
    this->cb_packet_(buf, len, static_cast<uint32_t>(this->udp_.remoteIP()), this->udp_.remotePort());
  }
}
#endif

#ifdef USE_ESP32
bool ESPAsyncUDP32::listen_multicast(const char *group_addr, uint16_t port) {
  this->close();

  this->sock_ = lwip_socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
  if (this->sock_ < 0) {
    ESP_LOGW(TAG, "socket failed: errno %d, %s", errno, strerror(errno));
    return false;
  }

  const int opt_enable = 1;
  int err = lwip_setsockopt(this->sock_, SOL_SOCKET, SO_REUSEADDR, &opt_enable, sizeof(opt_enable));
  if (err < 0) {
    ESP_LOGW(TAG, "setsockopt failed: errno %d, %s", errno, strerror(errno));
    // we can still continue
  }

  // set non-blocking mode
  lwip_fcntl(this->sock_, F_SETFL, lwip_fcntl(this->sock_, F_GETFL, 0) | O_NONBLOCK);

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = ESPHOME_INADDR_ANY;  // htonl(INADDR_ANY);
  addr.sin_port = htons(port);

  err = lwip_bind(this->sock_, (sockaddr *) &addr, sizeof(addr));
  if (err < 0) {
    ESP_LOGW(TAG, "bind failed: errno %d, %s", errno, strerror(errno));
    lwip_close(this->sock_);
    this->sock_ = -1;
    return false;
  }

  ip_mreq mreq;
  mreq.imr_multiaddr.s_addr = inet_addr(group_addr);
  mreq.imr_interface.s_addr = ESPHOME_INADDR_ANY;  // htonl(INADDR_ANY);

  err = lwip_setsockopt(this->sock_, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
  if (err < 0) {
    ESP_LOGW(TAG, "setsockopt failed: errno %d, %s", errno, strerror(errno));
    lwip_close(this->sock_);
    this->sock_ = -1;
    return false;
  }

  return true;
}

void ESPAsyncUDP32::close() {
  if (this->sock_ < 0) {
    return;
  }
  lwip_close(this->sock_);
  this->sock_ = -1;
}

ssize_t ESPAsyncUDP32::sendto(const uint8_t *data, size_t size, uint32_t addr, uint16_t port) {
  sockaddr_in addr_in{};
  addr_in.sin_family = AF_INET;
  addr_in.sin_addr.s_addr = addr;
  addr_in.sin_port = htons(port);
  return lwip_sendto(this->sock_, data, size, 0, (sockaddr *) &addr_in, sizeof(addr_in));
}

void ESPAsyncUDP32::loop_(uint8_t *buf, size_t recv_buf_size) {
  sockaddr_in from_addr{};
  socklen_t from_len = sizeof(from_addr);

  auto len = lwip_recvfrom(this->sock_, buf, recv_buf_size, 0, (sockaddr *) &from_addr, &from_len);
  if (len < 0) {
    // non-blocking socket without data
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      return;
    }
    ESP_LOGW(TAG, "recvfrom failed: errno %d, %s", errno, strerror(errno));
    return;
  }

  ESP_LOGV(TAG, "Received packet from %s:%u: %s", inet_ntoa(from_addr.sin_addr), ntohs(from_addr.sin_port),
           format_hex_pretty(buf, len).c_str());

  if (this->cb_packet_) {
    this->cb_packet_(buf, len, from_addr.sin_addr.s_addr, ntohs(from_addr.sin_port));
  }
}

#endif

}  // namespace esp_async_udp
}  // namespace esphome
