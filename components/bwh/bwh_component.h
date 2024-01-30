#pragma once

#include "esphome/core/component.h"
#include "esphome/components/switch/switch.h"

#include "../rka_api/rka_component.h"
#include "bwh_api.h"

namespace esphome {
namespace bwh {

using BWHComponentBase = rka_api::RKAComponent<rka_api::rka_dev_type_t::BWH, BWHListener, BWHApi>;

class BWHComponent : public BWHComponentBase {
 public:
  explicit BWHComponent(BWHApi *api) : BWHComponentBase(api) {}
  void on_state(const bwh_state_t &state) override {}
};

}  // namespace bwh
}  // namespace esphome
