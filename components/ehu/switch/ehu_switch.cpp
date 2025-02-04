#include "esphome/core/log.h"

#include "ehu_switch.h"

namespace esphome {
namespace ehu {

static const char *const TAG = "ehu.switch";

void EHUPowerSwitch::dump_config() {
  LOG_SWITCH("", "Electrolux Humidifier", this);
  this->dump_config_(TAG);
}

}  // namespace ehu
}  // namespace esphome
