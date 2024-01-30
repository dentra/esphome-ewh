#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "../rka_api/rka_component.h"
#include "ets_api.h"

namespace esphome {
namespace ets {

using ETSComponentBase = rka_api::RKAComponent<rka_api::rka_dev_type_t::ETS, ETSListener, ETSApi, PollingComponent>;

class ETSComponent : public ETSComponentBase {
 public:
  explicit ETSComponent(ETSApi *api) : ETSComponentBase(api) {}

  void update() override { this->api_->request_state(); }

  void set_floor_temp(sensor::Sensor *floor_temp) { this->floor_temp_ = floor_temp; }

 protected:
  void dump_config_(const char *TAG) const;
  sensor::Sensor *floor_temp_{};
};

}  // namespace ets
}  // namespace esphome
