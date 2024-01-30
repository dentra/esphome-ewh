#pragma once

#include "esphome/core/automation.h"
#include "ewh_api.h"

namespace esphome {
namespace ewh {

class EWHUpdateTrigger : public EWHListener, public Trigger<const ewh_state_t &> {
 public:
  explicit EWHUpdateTrigger(EWHApi *parent) { parent->add_listener(this); }
  void on_state(const ewh_state_t &state) override { this->trigger(state); }
};

}  // namespace ewh
}  // namespace esphome
