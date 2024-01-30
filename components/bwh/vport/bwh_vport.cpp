#include "bwh_vport.h"

namespace esphome {
namespace bwh {

static const char *const TAG = "bwh_vport";

void BWHVPort::dump_config() { VPORT_UART_LOG("BWH"); }

}  // namespace bwh
}  // namespace esphome
