#pragma once

#include "esphome/core/defines.h"
#include "esphome/core/component.h"

#ifdef USE_TIME
#include "esphome/components/time/real_time_clock.h"
#endif

#ifdef USE_SWITCH
#include "esphome/components/switch/switch.h"
#endif
#ifdef USE_SENSOR
#include "esphome/components/sensor/sensor.h"
#endif

#include "../rka_api/rka_component.h"
#include "ewh_api.h"

namespace esphome {
namespace ewh {

#ifdef USE_SWITCH
#define LOG_EWH() LOG_SWITCH("  ", "BST Swtich", this->bst_)
#else
#define LOG_EWH()
#endif

using EWHComponentBase = rka_api::RKAComponent<rka_api::rka_dev_type_t::EWH, EWHListener, EWHApi>;

class EWHComponent : public EWHComponentBase {
 public:
#ifdef USE_SWITCH
  // BST (Bacteria Stop Technology) Switch.
  class BSTSwitch : public switch_::Switch {
   public:
    explicit BSTSwitch(EWHApi *api) : api_(api) {}
    void write_state(bool state) override { this->api_->set_bst(state); }

   protected:
    EWHApi *api_;
  };
#endif

  explicit EWHComponent(EWHApi *api) : EWHComponentBase(api) {}

  void dump_config() override;

#ifdef USE_TIME
  void set_time_id(esphome::time::RealTimeClock *time) { this->time_ = time; }
#endif
#ifdef USE_SWITCH
  void set_bst(switch_::Switch *bst) { this->bst_ = bst; }
#endif
#ifdef USE_SENSOR
  void set_error_code(sensor::Sensor *error_code) { this->error_code_ = error_code; }
#endif

  void on_state(const ewh_state_t &state) override;

  // void set_idle_temp_drop(uint8_t idle_temp_drop) { this->idle_temp_drop_ = idle_temp_drop; }

 protected:
#ifdef USE_SWITCH
  switch_::Switch *bst_{};
#endif
#ifdef USE_SENSOR
  sensor::Sensor *error_code_{};
#endif
#ifdef USE_TIME
  esphome::time::RealTimeClock *time_{};
#endif

  // uint8_t idle_temp_drop_ = {};
  // uint8_t prev_temp_ = {};
  // bool is_idle_(const ewh_state_t &state);
};

}  // namespace ewh
}  // namespace esphome
