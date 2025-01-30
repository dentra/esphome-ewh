#pragma once

#include <algorithm>

#include "esphome/core/log.h"
#include "esphome/core/component.h"

#include "rka_vport.h"
#include "rka_data.h"
#include "rka_io.h"

// #ifdef USE_API
// #include "esphome/components/api/custom_api_device.h"
// #endif

namespace esphome {
namespace rka_api {

namespace internal {
extern const char *const TAG_API;
}

class RKAListenerBase : public RKAVPortListener {
 public:
  // read PACKET_RSP_DEV_TYPE.
  virtual void on_dev_type(const rka_dev_type_t &dev_type) {}
  // read PACKET_REQ_SAVE_DATA.
  virtual void on_result(const rka_result_t &result) {}
  // read PACKET_RSP_LOAD_DATA.
  virtual void on_data(const rka_data_t &data) {}
  // read PACKET_CMD_ERROR
  virtual void on_error(const rka_error_t &error) {}

  // Read any packet.
  void on_frame(const rka_any_frame_t &frame, size_t size) override;

 protected:
  bool check_packet_size_(size_t frame_size, size_t expected_data_size) const;
};

template<class rka_state_t> class RKAListener : public RKAListenerBase {
 public:
  // read PACKET_CMD_STATE or PACKET_REQ_STATE.
  virtual void on_state(const rka_state_t &state) = 0;

  void on_state(const void *data, size_t size) {
    if (this->check_packet_size_(size, sizeof(rka_state_t))) {
      this->on_state(*static_cast<const rka_state_t *>(data));
    }
  }

  void on_frame(const rka_any_frame_t &frame, size_t size) override {
    if (frame.type == PACKET_RSP_STATE) {
      this->on_state(frame.data, size);
    } else {
      RKAListenerBase::on_frame(frame, size);
    }
  }
};

// FIXME check for VPort base
template<class vport_t /*, std::enable_if_t<std::is_base_of_v<vport::VPort, vport_t>, bool> = true*/> class RKAApi {
  using this_t = RKAApi<vport_t>;

 public:
  explicit RKAApi(vport_t *vport) : vport_(vport) {}

  void add_listener(RKAListenerBase *listener) { this->vport_->add_listener(listener); }

  void request_dev_type() {
    ESP_LOGD(internal::TAG_API, "Request dev type");
    this->write(PACKET_REQ_DEV_TYPE);
  }

  void request_state() {
    ESP_LOGD(internal::TAG_API, "Request state");
    this->write(PACKET_REQ_STATE);
  }

  void request_state_ex() {
    ESP_LOGD(internal::TAG_API, "Request state (extended)");
    struct {
      uint8_t x[2];
    } PACKED state_req{0x10, 0x04};
    this->write(PACKET_REQ_STATE, state_req);
  }

  /// Write operation using PACKET_REQ_SET_COMMAND.
  template<class T, esphome::enable_if_t<std::is_class<T>::value, bool> = true> void write_op(const T &data) {
    ESP_LOGD(internal::TAG_API, "Set state (%u) %s", T::SET_OPERATION,
             format_hex_pretty(reinterpret_cast<const uint8_t *>(&data), sizeof(data)).c_str());
    struct {
      uint8_t op;
      T data;
    } PACKED op{.op = T::SET_OPERATION, .data = data};
    this->write(PACKET_REQ_SET_COMMAND, op);
  }

  /// Write state using PACKET_REQ_SET_COMMAND.
  template<class T, esphome::enable_if_t<std::is_class<T>::value, bool> = true> void write_st(const T &data) {
    ESP_LOGD(internal::TAG_API, "Set state %s",
             format_hex_pretty(reinterpret_cast<const uint8_t *>(&data), sizeof(data)).c_str());
    this->write(PACKET_REQ_SET_COMMAND, data);
  }

  // void write_state(const T &data) {
  //   // execute request state on next loop cycle
  //   this->defer(TAG_API, [this]() { this->request_state(); });
  // }

  /// Write command of specified type.
  void write(uint8_t type) {
    // ESP_LOGD(internal::TAG_API, "Write %02X", type);
    this->write_(&type, sizeof(type));
  }

  /// Write command of specified type and data struct.
  template<class T, esphome::enable_if_t<std::is_class<T>::value, bool> = true>
  void write(uint8_t type, const T &data) {
    // ESP_LOGD(internal::TAG_API, "Write %02X: %s", type, format_hex_pretty(&data, sizeof(data)).c_str());
    rka_frame_t<T> frame{.type = type, .data = data};
    this->write_(&frame, sizeof(frame));
  }

  /// Write command of type from T::REQ_FRAME_TYPE and data struct.
  template<class T, esphome::enable_if_t<std::is_class<T>::value, bool> = true> void write(const T &data) {
    this->write(T::REQ_FRAME_TYPE, data);
  }

  /// Write command of specified type and byte data.
  void write_byte(uint8_t type, uint8_t data) {
    rka_frame_t<uint8_t> frame{.type = type, .data = data};
    this->write_(&frame, sizeof(frame));
  }

 protected:
  vport_t *vport_;

  void write_(const void *data, size_t size) { this->vport_->write(*static_cast<const rka_any_frame_t *>(data), size); }
};

}  // namespace rka_api
}  // namespace esphome
