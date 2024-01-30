#pragma once

#include "esphome/components/vport/vport_uart.h"
#include "rka_io.h"

namespace esphome {
namespace rka_api {

using RKAVPort = vport::VPort<rka_any_frame_t>;

template<class io_t> using RKAVPortUARTComponent = vport::VPortUARTComponent<io_t, rka_any_frame_t>;

using RKAVPortListener = vport::VPortListener<rka_any_frame_t>;

}  // namespace rka_api
}  // namespace esphome
