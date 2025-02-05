#include "esphome/core/log.h"
#include "esphome/core/helpers.h"

#include "ewh_api.h"

namespace esphome {
namespace ewh {

static const char *const TAG = "ewh_api";

void EWHApi::set_mode(ewh_mode_t::Mode mode, uint8_t target_temperature) const {
  this->set_mode(ewh_mode_t{
      .mode = mode,
      .temperature = target_temperature,
  });
}

void EWHApi::set_clock(uint8_t hours, uint8_t minutes) const {
  this->set_clock(ewh_clock_t{
      .hours = hours,
      .minutes = minutes,
  });
}

void EWHApi::set_timer(uint8_t hours, uint8_t minutes, uint8_t temperature, ewh_mode_t::Mode mode) const {
  this->set_timer(ewh_timer_t{
      .hours = hours,
      .minutes = minutes,
      .mode = mode,
      .temperature = temperature,
  });
}

void EWHApi::set_bst(bool value) const {
  this->set_bst(ewh_bst_t{
      .state = value ? ewh_bst_t::STATE_ON : ewh_bst_t::STATE_OFF,
  });
}

void EWHApi::set_mode(const ewh_mode_t &mode) const {
  ESP_LOGD(TAG, "Set mode to %u, temperature %u", mode.mode, mode.temperature);
  if (mode.temperature < MIN_TEMPERATURE || mode.temperature > MAX_TEMPERATURE) {
    ESP_LOGW(TAG, "Mode temperature must be in [%u:%u] range", MIN_TEMPERATURE, MAX_TEMPERATURE);
    return;
  }
  this->write_op(mode);
}

void EWHApi::set_timer(const ewh_timer_t &timer) const {
  ESP_LOGD(TAG, "Set timer at %02u:%02u, mode %u, temperature %u", timer.hours, timer.minutes, timer.mode,
           timer.temperature);
  if (timer.hours > 23) {
    ESP_LOGW(TAG, "Timer hours must be in [0:23] range");
    return;
  }
  if (timer.minutes > 59) {
    ESP_LOGW(TAG, "Timer minutes must be in [0:59] range");
    return;
  }
  if (timer.temperature < MIN_TEMPERATURE || timer.temperature > MAX_TEMPERATURE) {
    ESP_LOGW(TAG, "Timer temperature must be in [%u:%u] range", MIN_TEMPERATURE, MAX_TEMPERATURE);
    return;
  }
  this->write_op(timer);
}

void EWHApi::set_clock(const ewh_clock_t &clock) const {
  ESP_LOGD(TAG, "Set clock to %02u:%02u", clock.hours, clock.minutes);
  if (clock.hours > 23) {
    ESP_LOGW(TAG, "Clock hours must be in [0:23] range");
    return;
  }
  if (clock.minutes > 59) {
    ESP_LOGW(TAG, "Clock minutes must be in [0:59] range");
    return;
  }
  this->write_op(clock);
}

void EWHApi::set_bst(const ewh_bst_t &bst) const {
  ESP_LOGD(TAG, "Set BST to %s", ONOFF(bst.state != ewh_bst_t::STATE_OFF));
  this->write_op(bst);
}

}  // namespace ewh
}  // namespace esphome
