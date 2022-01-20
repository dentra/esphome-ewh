#include "esphome/core/log.h"
#include "esphome/core/helpers.h"

#include "ewh_component.h"

namespace esphome {
namespace ewh {

static const char *const TAG = "ewh.component";

void EWHComponent::dump_config() {
  LOG_SWITCH("  ", "BST Swtich", this->bst_);
  auto dev_type = format_hex_pretty(reinterpret_cast<const uint8_t *>(&this->dev_type_), sizeof(this->dev_type_));
  ESP_LOGCONFIG(TAG, "  Device Type: %s", dev_type.c_str());
}

// bool EWHComponent::is_idle_(const ewh_state_t &state) {
//   // off state means no consumption
//   if (state.state == ewh_state_t::STATE_OFF) {
//     return true;
//   }

//   // the temperature has reached the target
//   if (state.current_temperature > state.target_temperature) {
//     return true;
//   }

//   // no info about previous temperature
//   if (this->prev_temperature_ == 0) {
//     this->prev_temperature_ = state.current_temperature;
//     return false;
//   }

//   // the temperature rises and has reached the target
//   if (this->prev_temperature_ < state.current_temperature && state.current_temperature == state.target_temperature) {
//     return true;
//   }

//   // the temperature goes down and dropped idle_temp_drop_ degrees from the target
//   if (this->prev_temperature_ > state.current_temperature &&
//       state.current_temperature >= state.target_temperature - this->idle_temp_drop_) {
//     return true;
//   }

//   if (this->prev_temperature_ != state.current_temperature) {
//     this->prev_temperature_ = state.current_temperature;
//   }

//   return false;
// }

}  // namespace ewh
}  // namespace esphome
