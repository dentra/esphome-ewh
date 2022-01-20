#include "esphome/core/log.h"
#include "esphome/core/helpers.h"
#include "inttypes.h"
#include "../ewh/ewh_utils.h"
#include "ewh_cloud.h"

namespace esphome {
namespace ewh {

static const char *const TAG = "ewh_cloud";

#define MAC_FMT "%02X%02X%02X%02X%02X%02X"

#define SMARTLINK_PORT 48899
#define SMARTLINK_PACK "smartlinkfind"
#define SMARTLINK_RSLT "smart_config " MAC_FMT

#define AT_NDBGL "NDBGL=0,0"
#define AT_APPVER "APPVER"
#define AT_WSMAC "WSMAC"

#define RS_NDBGL "+ok\r\n"
#define RS_APPVER "=BAIGE-1.8-20170810\r\n"

void EWHCloud::setup() {
  bool mac_set = false;
  for (auto v : this->mac_) {
    if (v) {
      mac_set = true;
      break;
    }
  }

  if (!mac_set) {
    get_mac_address_raw(this->mac_);
  }
}

void EWHCloud::on_tcp_data(const uint8_t *data, uint32_t size) {
  if (size > 6 && data[0] == 'A' && data[1] == 'T' && data[2] == '+') {
    this->on_at_(reinterpret_cast<const char *>(data + 3), size);
  } else if (size > 3 && data[OFFSET_FRAME_HEADER] == FRAME_HEADER && size > data[OFFSET_SIZE] + 2) {
    this->on_aa_(data, size);

  } else {
    ESP_LOGW(TAG, "Recv unresolved sequence: %s", format_hex_pretty(data, size).c_str());
  }
}

void EWHCloud::on_at_(const char *cmd, uint32_t size) {
  if (strncmp(cmd, AT_NDBGL, sizeof(AT_NDBGL) - 1) == 0) {
    ESP_LOGV(TAG, "Recv AT+" AT_NDBGL);
    this->tcp_->add(RS_NDBGL, sizeof(RS_NDBGL) - 1);
    this->tcp_->send();
    return;
  }

  if (strncmp(cmd, AT_APPVER, sizeof(AT_APPVER) - 1) == 0) {
    ESP_LOGV(TAG, "Recv AT+" AT_APPVER);
    this->tcp_->add(RS_APPVER, sizeof(RS_APPVER) - 1);
    this->tcp_->send();
    return;
  }

  if (strncmp(cmd, AT_WSMAC, sizeof(AT_WSMAC) - 1) == 0) {
    ESP_LOGV(TAG, "Recv AT+" AT_WSMAC);
    auto rs_wsmac = str_sprintf("+ok=" MAC_FMT "\r\n\r\n", this->mac_[0], this->mac_[1], this->mac_[2], this->mac_[3],
                                this->mac_[4], this->mac_[5]);
    this->tcp_->add(rs_wsmac.c_str(), rs_wsmac.length());
    this->tcp_->send();
    return;
  }

  auto at = std::string(cmd, size - 3);
  std::replace(at.begin(), at.end(), '\r', '\0');
  std::replace(at.begin(), at.end(), '\n', '\0');
  ESP_LOGW(TAG, "Recv unknown AT+%s", at.c_str());
}

void EWHCloud::on_aa_(const uint8_t *data, uint32_t size) {
  uint8_t cmd = data[OFFSET_COMMAND];
  ESP_LOGV(TAG, "Recv %02X: %s CRC: %02X", cmd,
           format_hex_pretty(data + (OFFSET_COMMAND + 1), size - (OFFSET_COMMAND + 1) - 1).c_str(),
           data[data[OFFSET_SIZE] + 2]);

  // send configured uid or if not request device for it
  if (cmd == PACKET_REQ_LOAD_DATA && this->uid_.value > 0) {
    ewh_data_t val{};
    val.data[sizeof(ewh_data_t::data) - 3] = this->uid_.data[0];
    val.data[sizeof(ewh_data_t::data) - 2] = this->uid_.data[1];
    val.data[sizeof(ewh_data_t::data) - 1] = this->uid_.data[2];
    uint8_t buf[3]{FRAME_HEADER, static_cast<uint8_t>(sizeof(val)), PACKET_RSP_LOAD_DATA};
    uint8_t crc = ewh::crc8(buf[0], buf[1], buf[2], &val, sizeof(val));
    this->tcp_->add(reinterpret_cast<const char *>(buf), sizeof(buf));
    this->tcp_->add(reinterpret_cast<const char *>(&val), sizeof(val));
    this->tcp_->add(reinterpret_cast<const char *>(&crc), sizeof(crc));
    this->tcp_->send();
    return;
  }

  this->ewh_->write_raw(data, size);
}

void EWHCloud::read(const PacketType type, const void *data, uint32_t size) {
  if (!this->is_connected()) {
    return;
  }
  uint8_t buf[3]{FRAME_HEADER, static_cast<uint8_t>(size), type};
  uint8_t crc = ewh::crc8(buf[0], buf[1], buf[2], data, size);
  this->tcp_->add(reinterpret_cast<const char *>(buf), sizeof(buf));
  this->tcp_->add(reinterpret_cast<const char *>(data), size);
  this->tcp_->add(reinterpret_cast<const char *>(&crc), sizeof(crc));
  this->tcp_->send();
}

void EWHCloud::disconnect() {
  if (this->tcp_) {
    delete this->tcp_;
    this->tcp_ = nullptr;
  }
}

void EWHCloud::connect() {
  if (this->tcp_) {
    ESP_LOGD(TAG, "Already connected");
    return;
  }

  this->tcp_ = new AsyncClient();

  ESP_LOGD(TAG, "Connecting to %s:%u...", this->host_.c_str(), this->port_);
  if (this->tcp_->connect(this->host_.c_str(), this->port_)) {
    this->tcp_->onData(
        [](void *arg, AsyncClient *client, void *data, size_t size) {
          reinterpret_cast<EWHCloud *>(arg)->on_tcp_data(static_cast<uint8_t *>(data), size);
        },
        this);

    this->tcp_->onConnect([](void *, AsyncClient *) { ESP_LOGD(TAG, "Connection estabilished"); }, this);
    this->tcp_->onDisconnect([](void *, AsyncClient *) { ESP_LOGD(TAG, "Disconnected"); }, this);
  } else {
    ESP_LOGW(TAG, "Connection failed");
    this->disconnect();
  }
}

void EWHCloud::pair(bool enable) {
  delete this->udp_;
  this->udp_ = nullptr;

  if (!enable) {
    return;
  }

  this->udp_ = new AsyncUDP();

  if (!this->udp_->connect(IPAddress(0, 0, 0, 0), SMARTLINK_PORT)) {
    delete this->udp_;
    this->udp_ = nullptr;
  }

  this->udp_->onPacket([this](AsyncUDPPacket packet) {
    const auto size = packet.length();
    if (size == sizeof(SMARTLINK_PACK) - 1 && std::memcmp(packet.data(), SMARTLINK_PACK, size) == 0) {
      auto str = str_sprintf(SMARTLINK_RSLT, this->mac_[0], this->mac_[1], this->mac_[2], this->mac_[3], this->mac_[4],
                             this->mac_[5]);
      this->udp_->writeTo(reinterpret_cast<const uint8_t *>(str.c_str()), str.length(), packet.remoteIP(),
                          packet.remotePort());
    }
  });
}

}  // namespace ewh
}  // namespace esphome
