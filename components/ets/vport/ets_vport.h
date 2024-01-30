#pragma once

#include "../../rka_api/rka_vport.h"
#include "../../rka_api/rka_api.h"
#include "../ets_data.h"

namespace esphome {
namespace ets {

struct ets_config {
  static constexpr auto max_data_size =
      std::max(sizeof(ets_state_t), std::max(sizeof(ets_mode_t), esphome::rka_api::rka_max_data_size_t::value));
};

using ETSUartIO = rka_api::RKAUartIO<ets_config::max_data_size>;

class ETSVPort : public rka_api::RKAVPortUARTComponent<ETSUartIO> {
 public:
  explicit ETSVPort(ETSUartIO *io) : rka_api::RKAVPortUARTComponent<ETSUartIO>(io) {}
  void dump_config() override;
};

}  // namespace ets
}  // namespace esphome
