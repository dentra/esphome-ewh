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
  if (time.is_valid() && rka_api::clock_differs(state.clock_hours, state.clock_minutes, time.hour, time.minute)) {
    this->defer([this]() {
      auto time = this->time_->now();
      this->api_->set_clock(time.hour, time.minute);
    });
  }
#endif
#ifdef USE_SWITCH
  if (this->bst_ != nullptr) {
    this->bst_->publish_state(state.bst.state != ewh_bst_t::STATE_OFF);
  }
#endif
#ifdef USE_SENSOR
  if (this->error_code_) {
    if (std::isnan(this->error_code_->state) || static_cast<uint8_t>(this->error_code_->state) != state.error) {
      this->error_code_->publish_state(state.error);
    }
  }
#endif
}

}  // namespace ewh
}  // namespace esphome
