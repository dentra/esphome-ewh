#include "ehu_vport.h"

namespace esphome {
namespace ehu {

static const char *const TAG = "ehu_vport";

void EHUVPort::dump_config() { VPORT_UART_LOG("EHU"); }

}  // namespace ehu
}  // namespace esphome
