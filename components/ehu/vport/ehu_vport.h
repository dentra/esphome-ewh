#pragma once

#include "esphome/components/vport/vport_uart.h"
#include "../../rka_api/rka_api.h"
#include "../ehu_data.h"

namespace esphome {
namespace ehu {

struct ehu_config {
  static constexpr auto max_data_size = std::max(sizeof(ehu_state_t), esphome::rka_api::rka_max_data_size_t::value);
};

using EHUUartIO = rka_api::RKAUartIO<ehu_config::max_data_size>;

class EHUVPort : public rka_api::RKAVPortUARTComponent<EHUUartIO> {
 public:
  explicit EHUVPort(EHUUartIO *io) : rka_api::RKAVPortUARTComponent<EHUUartIO>(io) {}
  void dump_config() override;
};

}  // namespace ehu
}  // namespace esphome
