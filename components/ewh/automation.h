#pragma once

#include "esphome/core/automation.h"
#include "ewh.h"

namespace esphome {
namespace ewh {

class EWHUpdateTrigger : public EWHListener, public Trigger<const ewh_status_t &> {
 public:
  explicit EWHUpdateTrigger(ElectroluxWaterHeater *parent) { parent->add_listener(this); }

  void read(const ewh_status_t &status) override { this->trigger(status); }
};

}  // namespace ewh
}  // namespace esphome
