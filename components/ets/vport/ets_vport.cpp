#include "ets_vport.h"

namespace esphome {
namespace ets {

static const char *const TAG = "ets_vport";

void ETSVPort::dump_config() { VPORT_UART_LOG("ETS"); }

}  // namespace ets
}  // namespace esphome
