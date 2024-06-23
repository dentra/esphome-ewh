#include "esphome/core/log.h"
#include "esphome/core/helpers.h"
#include "esphome/core/defines.h"

#ifdef USE_OTA
#include "esphome/components/ota/ota_backend.h"
#endif

#include "../rka_api/rka_data.h"
#include "../rka_api/rka_io.h"

#include "rka_cloud.h"

namespace esphome {
namespace rka_cloud {

static const char *const TAG = "rka_cloud";

#define MAC_FMT "%02X%02X%02X%02X%02X%02X"

#define AT_NDBGL "NDBGL=0,0"
#define AT_APPVER "APPVER"
#define AT_WSMAC "WSMAC"

#define CRLF "\r\n"
static const char RS_NDBGL[] = "+ok" CRLF;
static const char RS_APPVER[] = "=BAIGE-1.8-20170810" CRLF;
static const char RS_WSMAC[] = "+ok=" MAC_FMT CRLF CRLF;

template<class TCP> class TCPAdapter {
 public:
  explicit TCPAdapter(TCP *tcp) : tcp_(tcp) {}
  void write_array(const uint8_t *data, size_t size) { this->tcp_->send(reinterpret_cast<const char *>(data), size); }
  void flush() {}

 protected:
  TCP *tcp_;
};

void RKACloud::dump_config() {
  ESP_LOGCONFIG(TAG, "RKA Cloud:");
  ESP_LOGCONFIG(TAG, "  Server: %s:%u", this->host_, this->port_);
  if (this->is_mac_set_()) {
    ESP_LOGCONFIG(TAG, "  MAC: %02X:%02X:%02X:%02X:%02X:%02X", this->mac_[0], this->mac_[1], this->mac_[2],
                  this->mac_[3], this->mac_[4], this->mac_[5]);
  }
  if (this->uid_.value != 0) {
    ESP_LOGCONFIG(TAG, "  UID: %02u%02u%02u%02u", this->uid_.data[0], this->uid_.data[1], this->uid_.data[2],
                  this->uid_.data[3]);
  }
}

bool RKACloud::is_mac_set_() const {
  for (auto v : this->mac_) {
    if (v) {
      return true;
    }
  }
  return false;
}

void RKACloud::setup() {
  if (!this->is_mac_set_()) {
    get_mac_address_raw(this->mac_);
  }
#ifdef USE_OTA
  auto *global_ota_callback = ota::get_global_ota_callback();
  global_ota_callback->add_on_state_callback([this](ota::OTAState state, float, uint8_t, ota::OTAComponent *) {
    if (state == ota::OTAState::OTA_STARTED) {
      this->disconnect();
      if (this->cloud_pair_) {
        this->cloud_pair_->publish_state(false);
      }
    }
  });
#endif
}

void RKACloud::on_data_(const void *vdata, size_t size) {
  auto data = static_cast<const uint8_t *>(vdata);
  while (size > 0) {
    // check is AT command
    if (size > 6 && data[0] == 'A' && data[1] == 'T' && data[2] == '+') {
      size_t len = this->process_at_(reinterpret_cast<const char *>(data + 3), size - 3) + 3;
      size -= len;
      data += len;
      continue;
    }

    auto req = reinterpret_cast<const io_t::rx_frame_t *>(data);
    if (!req->is_valid()) {
      ESP_LOGW(TAG, "Recv unresolved sequence: %s", format_hex_pretty(data, size).c_str());
      return;
    }
    size_t len = req->size + sizeof(io_t::rx_frame_t);
    this->process_aa_(req, len);
    size -= len;
    data += len;
  }
}

static size_t skip_until_rn(const char *cmd, size_t len) {
  size_t res = 0;
  while (len > 0) {
    if (*cmd == '\r' || *cmd == '\n') {
      break;
    } else {
      cmd++;
      len--;
      res++;
    }
  }
  return res;
}

static size_t skip_while_rn(const char *cmd, size_t len) {
  size_t res = 0;
  while (len > 0) {
    if (*cmd == '\r' || *cmd == '\n') {
      cmd++;
      len--;
      res++;
    } else {
      break;
    }
  }
  return res;
}

size_t RKACloud::process_at_(const char *cmd, size_t len) {
  constexpr auto at_ndbg_len = sizeof(AT_NDBGL) - 1;
  if (std::strncmp(cmd, AT_NDBGL, at_ndbg_len) == 0) {
    ESP_LOGV(TAG, "Recv AT+" AT_NDBGL);
    this->tcp_->send(RS_NDBGL, sizeof(RS_NDBGL) - 1);
    return at_ndbg_len + skip_while_rn(cmd + at_ndbg_len, len - at_ndbg_len);
  }

  constexpr auto at_appver_len = sizeof(AT_APPVER) - 1;
  if (std::strncmp(cmd, AT_APPVER, at_appver_len) == 0) {
    ESP_LOGV(TAG, "Recv AT+" AT_APPVER);
    this->tcp_->send(RS_APPVER, sizeof(RS_APPVER) - 1);
    return at_appver_len + skip_while_rn(cmd + at_appver_len, len - at_appver_len);
  }

  constexpr auto at_wsmac_len = sizeof(AT_WSMAC) - 1;
  if (std::strncmp(cmd, AT_WSMAC, at_wsmac_len) == 0) {
    ESP_LOGV(TAG, "Recv AT+" AT_WSMAC);
    auto rs_wsmac =
        str_sprintf(RS_WSMAC, this->mac_[0], this->mac_[1], this->mac_[2], this->mac_[3], this->mac_[4], this->mac_[5]);
    this->tcp_->send(rs_wsmac.c_str(), rs_wsmac.length());
    return at_wsmac_len + skip_while_rn(cmd + at_wsmac_len, len - at_wsmac_len);
  }

  size_t unk_len = skip_until_rn(cmd, len);
  ESP_LOGW(TAG, "Recv unknown AT+%s", std::string(cmd, unk_len).c_str());
  return unk_len + skip_while_rn(cmd + unk_len, len - unk_len);
}

void RKACloud::process_aa_(const io_t::rx_frame_t *req, size_t size) {
  if (!this->io_.check_crc(req)) {
    ESP_LOGW(TAG, "Recv wrong CRC sequence: %s",
             format_hex_pretty(reinterpret_cast<const uint8_t *>(req), size).c_str());
    return;
  }

  auto frame = reinterpret_cast<const rka_api::rka_any_frame_t *>(req->data);

  ESP_LOGV(TAG, "Recv %02X: %s", frame->type, format_hex_pretty(frame->data, req->size - sizeof(frame->type)).c_str());

  // special case for configured UID
  if (frame->type == rka_api::PACKET_REQ_LOAD_DATA && this->uid_.value > 0) {
    rka_api::rka_frame_t<rka_api::rka_data_t> val{.type = rka_api::PACKET_RSP_LOAD_DATA, .data = {}};
    for (size_t i = 0; i < sizeof(this->uid_.data); i++) {
      val.data.data[sizeof(rka_api::rka_data_t::data) - (sizeof(this->uid_.data) - i)] = this->uid_.data[i];
    }
    ESP_LOGVV(TAG, "Replaced UID: %s", format_hex_pretty(val.data.data, sizeof(val.data.data)).c_str());
    TCPAdapter<TCPClient> adapter(this->tcp_);
    this->io_.write(&adapter, val);
    return;
  }

  // pass request to device
  this->vport_->write(*frame, req->size);
}

void RKACloud::on_frame(const rka_api::rka_any_frame_t &frame, size_t size) {
  if (this->is_connected()) {
    ESP_LOGV(TAG, "Send %02X: %s", frame.type, format_hex_pretty(frame.data, size - sizeof(frame.type)).c_str());
    TCPAdapter<TCPClient> adapter(this->tcp_);
    this->io_.write(&adapter, reinterpret_cast<const uint8_t *>(&frame), size);
  }
}

void RKACloud::disconnect() {
  auto tcp = this->tcp_;
  this->tcp_ = nullptr;
  delete tcp;
}

void RKACloud::connect() {
  if (this->tcp_) {
    ESP_LOGD(TAG, "Already connected");
    return;
  }

  ESP_LOGD(TAG, "Connecting to %s:%u...", this->host_, this->port_);
  this->tcp_ = new TCPClient();
  this->tcp_->on_connect(TCPClient::on_conn_type::create<RKACloud, &RKACloud::on_connect_>(*this));
  this->tcp_->on_disconnect(TCPClient::on_conn_type::create<RKACloud, &RKACloud::on_disconnect_>(*this));
  this->tcp_->on_data(TCPClient::on_data_type::create<RKACloud, &RKACloud::on_data_>(*this));
  if (!this->tcp_->connect(this->host_, this->port_)) {
    ESP_LOGW(TAG, "Connection failed");
    this->disconnect();
  }
}

void RKACloud::on_connect_() {
  ESP_LOGD(TAG, "Connection estabilished");
  this->publish_state(true);
}

void RKACloud::on_disconnect_() {
  ESP_LOGD(TAG, "Connection closed");
  this->publish_state(false);
}

}  // namespace rka_cloud
}  // namespace esphome
