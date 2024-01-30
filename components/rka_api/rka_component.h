#pragma once
#include <cinttypes>

#include "esphome/core/component.h"
#include "rka_api.h"

namespace esphome {
namespace rka_api {

namespace internal {
extern const char *const TAG_COMPONENT;
}

// TODO check listener type
template<uint16_t dev_type_v, class listener_t, class api_t, class component_t = Component>
class RKAComponent : public component_t, public listener_t {
 public:
  explicit RKAComponent(api_t *api) : api_(api) { this->api_->add_listener(this); }

  void setup() override {
    this->defer([this] { this->api_->request_dev_type(); });
  }

  float get_setup_priority() const override { return setup_priority::AFTER_WIFI; }

  void on_dev_type(const rka_dev_type_t &dev_type) override {
    if (dev_type.unknown != 0) {
      ESP_LOGW(internal::TAG_COMPONENT, "rka_dev_type_t.unknown, actual %08" PRIX32 ", expected 0x00000000",
               dev_type.unknown);
      this->mark_failed();
    }
    if (dev_type.type != dev_type_v) {
      ESP_LOGW(internal::TAG_COMPONENT, "rka_dev_type_t.type, actual %04X, expected %04X", dev_type.type, dev_type_v);
      this->mark_failed();
    }
  }

  void on_result(const rka_result_t &result) override {
    if (result.result != rka_result_t::RESULT_OK) {
      ESP_LOGW(internal::TAG_COMPONENT, "rka_result_t.result, actual %02X, expected %02X", result.result,
               rka_result_t::RESULT_OK);
    }
  }

  void on_error(const rka_error_t &error) override {
    switch (error.code) {
      case rka_error_t::CODE_BAD_CRC:
        ESP_LOGW(internal::TAG_COMPONENT, "Operation failed, invalid CRC");
        break;
      case rka_error_t::CODE_BAD_COMMAND:
        ESP_LOGW(internal::TAG_COMPONENT, "Operation failed, invalid command");
        break;
      default:
        ESP_LOGW(internal::TAG_COMPONENT, "Operation failed, code %02X", error.code);
        break;
    }
  }

  api_t *get_api() const { return this->api_; }

 protected:
  api_t *api_;
};

}  // namespace rka_api
}  // namespace esphome
