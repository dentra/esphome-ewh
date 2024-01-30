#pragma once

#include "../rka_api/rka_data.h"

// https://elux-ru.ru/upload/iblock/0d6/manual_centurio_iq_2_0.pdf

namespace esphome {
namespace ewh {

enum {
  MIN_TEMPERATURE = 35,
  MAX_TEMPERATURE = 75,
};

#pragma pack(push, 1)

struct ewh_mode_t {
  enum { SET_OPERATION = 0 };
  enum Mode : uint8_t { MODE_OFF = 0, MODE_700W = 1, MODE_1300W = 2, MODE_2000W = 3, MODE_NO_FROST = 4 } mode;
  uint8_t temperature;
};

struct ewh_clock_t {
  enum { SET_OPERATION = 1 };
  uint8_t hours;
  uint8_t minutes;
};

struct ewh_timer_t {
  enum { SET_OPERATION = 2 };
  uint8_t hours;
  uint8_t minutes;
  ewh_mode_t::Mode mode;  // from app it always 1
  uint8_t temperature;
};

// Bacteria Stop Technology.
struct ewh_bst_t {
  enum { SET_OPERATION = 3 };
  enum State : uint8_t { STATE_OFF = 0, STATE_ON = 1 } state;
};

// Response for PACKET_REQ_STATE and PACKET_STATUS.
struct ewh_state_t {
  enum State : uint8_t {
    STATE_OFF = 0,
    STATE_700W = 1,
    STATE_1300W = 2,
    STATE_2000W = 3,
    STATE_TIMER = 4,  // aka hot water preparation mode
    STATE_NO_FROST = 5
  } state;
  // temperature in celcius
  uint8_t current_temperature;
  uint8_t target_temperature;
  uint8_t clock_hours;
  uint8_t clock_minutes;
  uint8_t timer_hours;
  uint8_t timer_minutes;
  uint8_t error;  // 0 - no error, 1 - temperature sensor error, 2-255 - other error
  ewh_bst_t bst;
};

#pragma pack(pop)

}  // namespace ewh
}  // namespace esphome
