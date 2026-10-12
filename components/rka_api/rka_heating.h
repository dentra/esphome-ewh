#pragma once

#include <cstdint>

namespace esphome {
namespace rka_api {

/// Estimates water heater heating element state by temperature changes.
/// Device does not report heating element state. It heats until target temperature is reached
/// and does not heat until water cools down by some degrees.
class HeatingEstimator {
 public:
  void set_idle_temp_drop(uint8_t idle_temp_drop) { this->idle_temp_drop_ = idle_temp_drop; }
  uint8_t get_idle_temp_drop() const { return this->idle_temp_drop_; }

  /// Update estimation with device state, returns true when heating.
  bool update(bool is_on, uint8_t current, uint8_t target) {
    if (!is_on) {
      this->heating_ = false;
    } else if (current >= target) {
      // target temperature reached
      this->heating_ = false;
    } else if (target - current > this->idle_temp_drop_) {
      // cooled down too much, heating element must be on
      this->heating_ = true;
    } else if (this->heating_) {
      // heats until target temperature is reached, even if hot water is used
    } else if (this->has_prev_ && !this->prev_on_) {
      // just turned on
      this->heating_ = true;
    } else if (this->has_prev_ && target > this->prev_target_) {
      // target temperature raised
      this->heating_ = true;
    } else if (this->has_prev_ && current > this->prev_) {
      // temperature rises
      this->heating_ = true;
    }
    // otherwise cools down within idle range or state is unknown yet
    this->prev_ = current;
    this->prev_target_ = target;
    this->prev_on_ = is_on;
    this->has_prev_ = true;
    return this->heating_;
  }

  bool is_heating() const { return this->heating_; }

 protected:
  uint8_t idle_temp_drop_{5};
  uint8_t prev_{};
  uint8_t prev_target_{};
  bool prev_on_{};
  bool has_prev_{};
  bool heating_{};
};

}  // namespace rka_api
}  // namespace esphome
