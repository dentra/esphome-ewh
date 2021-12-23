#pragma once

#include <stdint.h>

// https://elux-ru.ru/upload/iblock/0d6/manual_centurio_iq_2_0.pdf

#pragma pack(push, 1)

enum {
  MIN_TEMPERATURE = 35,
  MAX_TEMPERATURE = 75,
};

enum : uint8_t { FRAME_HEADER = 0xAA };

enum PacketType : uint8_t {
  /**
   * Unknown zero-bdy command.
   */
  PACKET_REQ_UNKNOWN01 = 0x01,

  /** */
  PACKET_REQ_UNKNOWN06 = 0x06,

  /**
   * Unknown zero-bdy command.
   */
  PACKET_REQ_UNKNOWN07 = 0x07,

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
   * Result of PACKET_REQ_UNKNOWN01.
   * @returns 00.00.00.00.00.11
   */
  PACKET_RSP_UNKNOWN81 = _PACKET_RSP_CMD_MASK | PACKET_REQ_UNKNOWN01,
  /**
   * Result of PACKET_REQ_UNKNOWN06.
   * @returns 01
   */
  PACKET_RSP_UNKNOWN86 = _PACKET_RSP_CMD_MASK | PACKET_REQ_UNKNOWN06,
  /**
   * Result of PACKET_REQ_UNKNOWN07.
   * First time returns 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
   * @returns 00.00.00.00.00.00.00.00.00.00.00.00.00.1A.07.32 (0xA1=161, 0x07=7, 0x32=50)
   */
  PACKET_RSP_UNKNOWN87 = _PACKET_RSP_CMD_MASK | PACKET_REQ_UNKNOWN07,

  /**
   * Result of PACKET_REQ_STATE
   * @returns wh_state_t
   */
  PACKET_RSP_STATE = _PACKET_RSP_CMD_MASK | PACKET_REQ_STATE,

  /**
   * Device status. Got every 30 seconds.
   * @returns wh_state_t
   */
  PACKET_RSP_STATUS = 0x09,

  /**
   * Error executing previous command.
   * @returns wh_error_t
   */
  PACKET_RSP_ERROR = 0x05,
};

struct ewh_mode_t {
  enum : uint8_t { SET_OPERATION = 0 };
  enum : uint8_t { MODE_OFF = 0, MODE_700W = 1, MODE_1300W = 2, MODE_2000W = 3, MODE_NO_FROST = 4 } mode;
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
  uint8_t mode;  // from app it always 1
  uint8_t temperature;
};

// Bacteria Stop Technology.
struct ewh_bst_t {
  enum : uint8_t { SET_OPERATION = 3 };
  enum : uint8_t { STATE_OFF = 0, STATE_ON = 1 } state;
};

struct ewh_error_t {
  enum : uint8_t { CODE_BAD_CRC = 1, CODE_BAD_COMMAND = 2 } code;
};

struct ewh_state_t {
  enum : uint8_t {
    STATE_OFF = 0,
    STATE_700W = 1,
    STATE_1300W = 2,
    STATE_2000W = 3,
    STATE_TIMER = 4,  // hot water preparation mode
    STATE_NO_FROST = 5
  } state;
  // temperature in celcius
  uint8_t current_temperature;
  uint8_t target_temperature;
  uint8_t clock_hours;
  uint8_t clock_minutes;
  uint8_t timer_hours;
  uint8_t timer_minutes;
  uint8_t unknown;
  ewh_bst_t bst;  // 0-off, 1-enabled, возможно другая цифра, когда режим активен.
};

// Response 81
// 00.00.00.00.00.11
struct ewh_unknown81 {
  uint8_t unknown00[5];  // always {0,0,0,0,0}
  uint8_t unknown11;     // always 11
};

// Response 86
// 01
struct ewh_unknown86 {
  uint8_t unknown01;  // always 01
};

// Response 87 and probably Request 06
// 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
// 00.00.00.00.00.00.00.00.00.00.00.00.00.1A.07.32
// 00.00.00.00.00.00.00.00.B9.00.00.00.00.00.00.00
struct ewh_unknown87 {
  uint8_t unknown00_8[8];  // always {0,0,0,0,0,0,0,0}
  uint8_t unknownB9;       // 00 or B9
  uint8_t unknown00_4[4];  // always {0,0,0,0}
  uint8_t unknown1A;       // 00 or 1A
  uint8_t unknown07;       // 00 or 07
  uint8_t unknown32;       // 00 or 32
};

#pragma pack(pop)
