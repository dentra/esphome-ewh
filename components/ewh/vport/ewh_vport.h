#pragma once

#include "esphome/components/vport/vport_uart.h"
#include "../../rka_api/rka_api.h"
#include "../ewh_data.h"

namespace esphome {
namespace ewh {

struct ewh_config {
  static constexpr auto max_data_size = std::max(sizeof(ewh_state_t), esphome::rka_api::rka_max_data_size_t::value);
};

using EWHUartIO = rka_api::RKAUartIO<ewh_config::max_data_size>;

class EWHVPort : public rka_api::RKAVPortUARTComponent<EWHUartIO> {
 public:
  explicit EWHVPort(EWHUartIO *io) : rka_api::RKAVPortUARTComponent<EWHUartIO>(io) {}
  void dump_config() override;
};

}  // namespace ewh
}  // namespace esphome
