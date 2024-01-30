#include "esphome/core/log.h"
#include "esphome/core/helpers.h"
#include "esphome/core/defines.h"

#include "rka_pair.h"

namespace esphome {
namespace rka_cloud {
static const char *const TAG = "rka_pair";

#define MAC_FMT "%02X%02X%02X%02X%02X%02X"

#define SMARTLINK_GROUP "239.48.0.0"
#define SMARTLINK_PORT 48899
#define SMARTLINK_PACK "smartlinkfind"
#define SMARTLINK_RSLT "smart_config " MAC_FMT

void RKACloudPair::pair_(bool enable) {
  delete this->udp_;
  this->udp_ = nullptr;

  if (!enable) {
    ESP_LOGD(TAG, "Pairing disabled");
    return;
  }

  this->cancel_timeout(TAG);

  this->udp_ = new UDPServer();
  this->udp_->on_packet(UDPServer::on_packet_type::create<RKACloudPair, &RKACloudPair::on_packet_>(*this));
  if (!this->udp_->listen_multicast(SMARTLINK_GROUP, SMARTLINK_PORT)) {
    delete this->udp_;
    this->udp_ = nullptr;
    ESP_LOGW(TAG, "Pairing failed");
    return;
  }

  ESP_LOGD(TAG, "Pairing enabled");
}

void RKACloudPair::on_packet_(const uint8_t *data, size_t size, uint32_t addr, uint16_t port) {
  if (size != sizeof(SMARTLINK_PACK) - 1 || std::memcmp(data, SMARTLINK_PACK, size) != 0) {
    ESP_LOGW(TAG, "Unknow packet: %s", format_hex_pretty(data, size).c_str());
    return;
  }
  ESP_LOGD(TAG, "Got smartlink packet");
  auto mac = this->cloud_->get_mac();
  auto str = str_sprintf(SMARTLINK_RSLT, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  ESP_LOGV(TAG, "Packet to send: %s", str.c_str());
  auto buf = reinterpret_cast<const uint8_t *>(str.c_str());
  auto len = str.length();
  this->udp_->sendto(buf, len, addr, port);
  ESP_LOGI(TAG, "Pairing success");

  this->set_timeout(TAG, 3000, [this]() {
    delete this->udp_;
    this->udp_ = nullptr;
    this->publish_state(false);
  });
}

}  // namespace rka_cloud
}  // namespace esphome
