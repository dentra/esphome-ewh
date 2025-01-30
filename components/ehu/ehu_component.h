#pragma once

#include "esphome/core/component.h"
#include "esphome/components/switch/switch.h"

#include "../rka_api/rka_component.h"
#include "ehu_api.h"

namespace esphome {
namespace ehu {

using EHUComponentBase = rka_api::RKAComponent<rka_api::rka_dev_type_t::EHU, EHUListener, EHUApi, PollingComponent>;

class EHUComponent : public EHUComponentBase {
 public:
  explicit EHUComponent(EHUApi *api) : EHUComponentBase(api) {}
  void on_state(const ehu_state_t &state) override {}

  void update() override { this->api_->request_state_ex(); }
};

}  // namespace ehu
}  // namespace esphome
