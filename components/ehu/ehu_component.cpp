#include "esphome/core/log.h"

#include "ehu_component.h"

namespace esphome {
namespace ehu {

void EHUComponent::on_state(const ehu_state_t &state) {
  this->publish_state_(this->temperature_, state.temperature);
  this->publish_state_(this->humidity_, state.humidity);
  this->publish_state_(this->warm_mist_, state.water_flags & ehu_state_t::WATER_WARM_MIST);
  this->publish_state_(this->uv_, state.water_flags & ehu_state_t::WATER_UV);
  this->publish_state_(this->ionizer_, state.ionizer);
  this->publish_state_(this->lock_, state.lock);
  this->publish_state_(this->sound_, state.sound);
  this->publish_state_(this->water_, !state.water_tank_empty);
}

}  // namespace ehu
}  // namespace esphome
