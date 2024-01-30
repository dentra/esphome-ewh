#pragma once

#include "esphome/components/vport/vport_uart.h"
#include "../../rka_api/rka_api.h"
#include "../bwh_data.h"

namespace esphome {
namespace bwh {

struct bwh_config {
  static constexpr auto max_data_size = std::max(sizeof(bwh_state_t), esphome::rka_api::rka_max_data_size_t::value);
};

using BWHUartIO = rka_api::RKAUartIO<bwh_config::max_data_size>;

class BWHVPort : public rka_api::RKAVPortUARTComponent<BWHUartIO> {
 public:
  explicit BWHVPort(BWHUartIO *io) : rka_api::RKAVPortUARTComponent<BWHUartIO>(io) {}
  void dump_config() override;
};

}  // namespace bwh
}  // namespace esphome
