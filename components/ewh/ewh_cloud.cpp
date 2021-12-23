#include "esphome/core/log.h"
#include "esphome/core/helpers.h"

#include "ewh.h"
#include "ewh_cloud.h"
#include "ewh_component.h"

namespace esphome {
namespace ewh {

static const char *const TAG = "ewh.cloud";

#define AT_NDBGL "NDBGL=0,0"
#define AT_APPVER "APPVER"
#define AT_WSMAC "WSMAC"

#define RS_NDBGL "+ok\r\n"
#define RS_WSMAC "+ok=" EWH_CLOUD_MAC "\r\n\r\n"
#define RS_APPVER "=BAIGE-1.8-20170810\r\n"

void EWHCloud::on_cloud_data(const uint8_t *data, uint32_t size) {
#ifdef EWH_CLOUD_ENABLE
  if (size > 6 && data[0] == 'A' && data[1] == 'T' && data[2] == '+') {
    auto start = reinterpret_cast<const char *>(data + 3);
    if (false) {
      // just pretty print
    } else if (strncmp(start, AT_NDBGL, sizeof(AT_NDBGL) - 1) == 0) {
      ESP_LOGV(TAG, "Recv AT+" AT_NDBGL);
      this->cloud_client_->add(RS_NDBGL, sizeof(RS_NDBGL) - 1);
      this->cloud_client_->send();
    } else if (strncmp(start, AT_APPVER, sizeof(AT_APPVER) - 1) == 0) {
      ESP_LOGV(TAG, "Recv AT+" AT_APPVER);
      this->cloud_client_->add(RS_APPVER, sizeof(RS_APPVER) - 1);
      this->cloud_client_->send();
    } else if (strncmp(start, AT_WSMAC, sizeof(AT_WSMAC) - 1) == 0) {
      ESP_LOGV(TAG, "Recv AT+" AT_WSMAC);
      this->cloud_client_->add(RS_WSMAC, sizeof(RS_WSMAC) - 1);
      this->cloud_client_->send();
    } else {
      auto at = std::string(start, size - 3);
      std::replace(at.begin(), at.end(), '\r', '\0');
      std::replace(at.begin(), at.end(), '\n', '\0');
      ESP_LOGW(TAG, "Recv unknown AT+%s", at.c_str());
    }
  } else if (size > 3 && data[0] == FRAME_HEADER && size > data[1] + 2) {
    ESP_LOGV(TAG, "Recv %02X: %s CRC: %02X", data[2], hexencode(data + 3, size - 4).c_str(), data[data[1] + 2]);
    this->on_cloud_frame(static_cast<PacketType>(data[2]), data + 3, size - 4);
    this->ewh_->write_array(data, size);
    this->ewh_->flush();
  } else {
    ESP_LOGW(TAG, "Recv unresolved sequence: %s", hexencode(data, size).c_str());
  }
#endif
}

void EWHCloud::on_device_frame(const uint8_t *data, uint32_t size, uint8_t crc) {
#ifdef EWH_CLOUD_ENABLE
  if (!this->is_connected()) {
    return;
  }
  char buf[2] = {FRAME_HEADER, static_cast<uint8_t>(size)};
  this->cloud_client_->add(buf, sizeof(buf));
  this->cloud_client_->add(reinterpret_cast<const char *>(data), size);
  this->cloud_client_->add(reinterpret_cast<const char *>(&crc), sizeof(crc));
  this->cloud_client_->send();
#endif
}

void EWHCloud::on_cloud_frame(const PacketType type, const uint8_t *data, uint32_t size) {
#ifdef EWH_CLOUD_ENABLE
  auto debug_sens = this->ewh_->get_debug();
  if (debug_sens == nullptr) {
    return;
  }

  if (type == PACKET_REQ_STATE && size == 2 && data[0] == 0x10 && data[1] == 0x04) {
    return;
  }

  if (type == PACKET_REQ_UNKNOWN01 && size == 0) {
    return;
  }

  if (type == PACKET_REQ_UNKNOWN07 && size == 0) {
    return;
  }

  ESP_LOGW(TAG, "Unknown cloud command %02X: %s", type, hexencode(data, size).c_str());

  auto debug = str_sprintf("CLD %02X: %s", type, hexencode(data, size).c_str());
  if (debug_sens->state != debug) {
    debug_sens->publish_state(debug);
  }
#endif
}

bool EWHCloud::is_connected() {
#ifdef EWH_CLOUD_ENABLE
  if (this->cloud_client_) {
    return this->cloud_client_->connected();
  }
#endif
  return false;
}

void EWHCloud::disconnect() {
#ifdef EWH_CLOUD_ENABLE
  if (this->cloud_client_) {
    delete this->cloud_client_;
    this->cloud_client_ = nullptr;
  }
#endif
}

void EWHCloud::connect() {
#ifdef EWH_CLOUD_ENABLE
  if (this->cloud_client_) {
    delete this->cloud_client_;
  }
  this->cloud_client_ = new AsyncClient;
  this->cloud_client_->onData(
      [](void *arg, AsyncClient *client, void *data, size_t size) {
        reinterpret_cast<EWHCloud *>(arg)->on_cloud_data(static_cast<uint8_t *>(data), size);
      },
      this);

  this->cloud_client_->onConnect([](void *, AsyncClient *) { ESP_LOGD(TAG, "Connection estabilished"); }, this);

  ESP_LOGD(TAG, "Connecting to %s:%u...", EWH_CLOUD_HOST, EWH_CLOUD_PORT);
  this->cloud_client_->connect(EWH_CLOUD_HOST, EWH_CLOUD_PORT);
#endif
}

}  // namespace ewh
}  // namespace esphome
