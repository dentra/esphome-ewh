#include "ewh_vport.h"

namespace esphome {
namespace ewh {

static const char *const TAG = "ewh_vport";

void EWHVPort::dump_config() { VPORT_UART_LOG("EWH"); }

}  // namespace ewh
}  // namespace esphome
