#pragma once

#include "esphome/core/automation.h"
#include "ets_api.h"

namespace esphome {
namespace ets {

class ETSUpdateTrigger : public ETSListener, public Trigger<const ets_state_t &> {
 public:
  explicit ETSUpdateTrigger(ETSApi *parent) { parent->add_listener(this); }
  void on_state(const ets_state_t &status) override { this->trigger(status); }
};

}  // namespace ets
}  // namespace esphome
