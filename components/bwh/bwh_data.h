#pragma once

#include "../rka_api/rka_data.h"

namespace esphome {
namespace bwh {

enum {
  MIN_TEMPERATURE = 35,
  MAX_TEMPERATURE = 75,
};

#pragma pack(push, 1)

struct bwh_mode_t {
  enum : uint8_t { SET_OPERATION = 0 };
  enum Mode : uint8_t { MODE_OFF = 0, MODE_1300W = 1, MODE_2000W = 2 } mode;
  uint8_t temperature;
};

// Response for PACKET_REQ_STATE and PACKET_STATUS.
struct bwh_state_t {
  enum State : uint8_t {
    STATE_OFF = 0,
    STATE_1300W = 1,
    STATE_2000W = 2,
  } state;
  // temperature in celcius
  uint8_t current_temperature;
  uint8_t target_temperature;
  uint8_t unknown[4];
  uint8_t error;  // 0 - no error, 1 - temperature sensor error, 2-255 - other error
};

#pragma pack(pop)

}  // namespace bwh
}  // namespace esphome
