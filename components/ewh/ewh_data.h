#pragma once

#include <stdint.h>

// https://elux-ru.ru/upload/iblock/0d6/manual_centurio_iq_2_0.pdf

namespace esphome {
namespace ewh {

#pragma pack(push, 1)

enum {
  MIN_TEMPERATURE = 35,
  MAX_TEMPERATURE = 75,
};

enum : uint8_t {
  FRAME_HEADER = 0xAA,

  OFFSET_FRAME_HEADER = 0,
  OFFSET_SIZE = 1,
  OFFSET_COMMAND = 2,
};

enum PacketType : uint8_t {
  /**
   * Possibly device type request.
   */
  PACKET_REQ_DEV_TYPE = 0x01,

  /**
   * Request to save wh_data structure.
   */
  PACKET_REQ_SAVE_DATA = 0x06,

  /**
   * Request to load previosly saved wh_data structure.
   */
  PACKET_REQ_LOAD_DATA = 0x07,

  /**
   * Request device state.
   */
  PACKET_REQ_STATE = 0x08,

  /**
   * Request change device state.
   *
   * First byte is operation, other one of following struct:
   *  - ewh_mode_t
   *  - ewh_clock_t
   *  - ewh_timer_t
   *  - ewh_bst_t
   */
  PACKET_REQ_SET_COMMAND = 0x0A,

  _PACKET_RSP_CMD_MASK = 0x80,

  /**
   * Result of PACKET_REQ_DEV_TYPE.
   * @returns ewh_dev_type_t
   */
  PACKET_RSP_DEV_TYPE = _PACKET_RSP_CMD_MASK | PACKET_REQ_DEV_TYPE,
  /**
   * Result of PACKET_REQ_SAVE_DATA.
   * @returns ewh_result_t
   */
  PACKET_RSP_SAVE_DATA = _PACKET_RSP_CMD_MASK | PACKET_REQ_SAVE_DATA,
  /**
   * Result of PACKET_REQ_LOAD_DATA.
   * @returns wh_data_t
   */
  PACKET_RSP_LOAD_DATA = _PACKET_RSP_CMD_MASK | PACKET_REQ_LOAD_DATA,

  /**
   * Result of PACKET_REQ_STATE.
   * @returns wh_state_t
   */
  PACKET_RSP_STATE = _PACKET_RSP_CMD_MASK | PACKET_REQ_STATE,

  /**
   * Device status. Got every 30 seconds.
   * @returns wh_state_t
   */
  PACKET_STATUS = 0x09,

  /**
   * Error executing previous command.
   * @returns wh_error_t
   */
  PACKET_ERROR = 0x05,
};

struct ewh_mode_t {
  enum : uint8_t { SET_OPERATION = 0 };
  enum Mode : uint8_t { MODE_OFF = 0, MODE_700W = 1, MODE_1300W = 2, MODE_2000W = 3, MODE_NO_FROST = 4 } mode;
  uint8_t temperature;
};

struct ewh_clock_t {
  enum : uint8_t { SET_OPERATION = 1 };
  uint8_t hours;
  uint8_t minutes;
};

struct ewh_timer_t {
  enum : uint8_t { SET_OPERATION = 2 };
  uint8_t hours;
  uint8_t minutes;
  ewh_mode_t::Mode mode;  // from app it always 1
  uint8_t temperature;
};

// Bacteria Stop Technology.
struct ewh_bst_t {
  enum : uint8_t { SET_OPERATION = 3 };
  enum State : uint8_t { STATE_OFF = 0, STATE_ON = 1 } state;
};

// Response for PACKET_ERROR.
struct ewh_error_t {
  enum Code : uint8_t { CODE_BAD_CRC = 1, CODE_BAD_COMMAND = 2 } code;
};

// Response for PACKET_STATUS.
struct ewh_status_t {
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
  uint8_t unknown;  // always 0
  ewh_bst_t bst;
};

// Response PACKET_REQ_STATE. Same as ewh_status_t.
struct ewh_state_t : ewh_status_t {};

// Response for PACKET_RSP_DEV_TYPE.
struct ewh_dev_type_t {
  uint32_t unknown;                       // always 0
  enum : uint16_t { EWH = 0x1100 } type;  // always 0x1100
};

// Response for PACKET_REQ_SAVE_DATA.
struct ewh_result_t {
  enum : uint8_t { RESULT_OK = 1 };
  uint8_t result;
};

// Response for PACKET_RSP_LOAD_DATA and request for PACKET_REQ_SAVE_DATA.
struct ewh_data_t {
  uint8_t data[16];
};

#pragma pack(pop)

}  // namespace ewh
}  // namespace esphome
