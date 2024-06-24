#include "esphome/core/log.h"
#include "esphome/core/helpers.h"

#include "ewh_component.h"

namespace esphome {
namespace ewh {

static const char *const TAG = "ewh.component";

void EWHComponent::dump_config() { LOG_EWH(); }

void EWHComponent::on_state(const ewh_state_t &state) {
#ifdef USE_TIME
  auto time = this->time_->now();
  if (time.is_valid() && state.clock_hours != time.hour && state.clock_minutes != time.minute) {
    this->api_->set_clock(time.hour, time.minute);
  }
#endif
#ifdef USE_SWITCH
  if (this->bst_ != nullptr) {
    this->bst_->publish_state(state.bst.state != ewh_bst_t::STATE_OFF);
  }
#endif
#ifdef USE_SENSOR
  if (this->error_code_) {
    this->error_code_->publish_state(state.error);
  }
#endif
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
