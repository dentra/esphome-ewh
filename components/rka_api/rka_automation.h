#pragma once

#include "esphome/core/automation.h"
#include "rka_api.h"

namespace esphome {
namespace rka_api {

template<class ApiT, class StateT> class RKAUpdateTrigger : public RKAListener<StateT>, public Trigger<const StateT &> {
 public:
  explicit RKAUpdateTrigger(ApiT *parent) { parent->add_listener(this); }
  void on_state(const StateT &state) override { this->trigger(state); }
};

}  // namespace rka_api
}  // namespace esphome
