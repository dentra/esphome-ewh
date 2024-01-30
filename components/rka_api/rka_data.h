#pragma once

#include <cstdint>
#include <cstddef>

namespace esphome {
namespace rka_api {

#pragma pack(push, 1)

enum rka_packet_type_t : uint8_t {
  /**
   * Device type request.
   */
  PACKET_REQ_DEV_TYPE = 0x01,

  /**
   * Request to save rka_data structure.
   */
  PACKET_REQ_SAVE_DATA = 0x06,

  /**
   * Request to load previosly saved rka_data structure.
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
   * @returns rka_dev_type_t
   */
  PACKET_RSP_DEV_TYPE = _PACKET_RSP_CMD_MASK | PACKET_REQ_DEV_TYPE,
  /**
   * Result of PACKET_REQ_SAVE_DATA.
   * @returns rka_result_t
   */
  PACKET_RSP_SAVE_DATA = _PACKET_RSP_CMD_MASK | PACKET_REQ_SAVE_DATA,
  /**
   * Result of PACKET_REQ_LOAD_DATA.
   * @returns rka_data_t
   */
  PACKET_RSP_LOAD_DATA = _PACKET_RSP_CMD_MASK | PACKET_REQ_LOAD_DATA,

  /**
   * Result of PACKET_REQ_STATE.
   * @returns rka_state_t
   */
  PACKET_RSP_STATE = _PACKET_RSP_CMD_MASK | PACKET_REQ_STATE,

  /**
   * Result of PACKET_REQ_SET_COMMAND.
   */
  PACKET_RSP_SET_COMMAND = _PACKET_RSP_CMD_MASK | PACKET_REQ_SET_COMMAND,

  /**
   * Device state command. EWH got it every 30 seconds.
   * @returns rka_state_t
   */
  PACKET_CMD_STATE = 0x09,

  /**
   * Error executing previous command.
   * @returns rka_error_t
   */
  PACKET_CMD_ERROR = 0x05,
};

// // Header for any request or response.
// struct frame_header_t {
//   enum : uint8_t { FRAME_MAGIC = 0xAA };
//   uint8_t magic;
//   // Size of frame data.
//   uint8_t size;
//   rka_packet_type_t type;
// };

// // Abstract frame with any data.
// // Usage:
// //  auto frame = any_frame_t::from(data);
// struct any_frame_t : frame_header_t {
//   uint8_t data[sizeof(uint8_t)];

//   /// Check frame is valid with magic and whole frame size (magic + size + data + CRC).
//   bool is_valid(size_t whole_frame_size) const {
//     return this->magic == FRAME_MAGIC && whole_frame_size - this->size - sizeof(frame_header_t) == 0;
//   }

//   /// Cast data pointer to frame pointer.
//   static const any_frame_t *from(const uint8_t *data) { return reinterpret_cast<const any_frame_t *>(data); }

//   uint8_t crc() const { return this->data[this->size - 1]; }
// };

// Response for PACKET_CMD_ERROR.
struct rka_error_t {
  enum {
    RSP_FRAME_TYPE = PACKET_CMD_ERROR,
  };
  enum Code : uint8_t { CODE_BAD_CRC = 1, CODE_BAD_COMMAND = 2 } code;
};

// Response for PACKET_RSP_DEV_TYPE.
struct rka_dev_type_t {
  enum {
    RSP_FRAME_TYPE = PACKET_RSP_DEV_TYPE,
  };
  uint32_t unknown;  // always 0
  enum : uint16_t { EWH = 0x1100, BWH = 0x0400, ETS = 0x0501 } type;
};

// Response for PACKET_REQ_SAVE_DATA.
struct rka_result_t {
  enum {
    RSP_FRAME_TYPE = PACKET_RSP_SAVE_DATA,
  };
  enum : uint8_t { RESULT_OK = 1 };
  uint8_t result;
};

// Response for PACKET_RSP_LOAD_DATA and request for PACKET_REQ_SAVE_DATA.
struct rka_data_t {
  enum {
    RSP_FRAME_TYPE = PACKET_RSP_LOAD_DATA,
    REQ_FRAME_TYPE = PACKET_REQ_SAVE_DATA,
  };
  uint8_t data[16];
};

#pragma pack(pop)

struct rka_max_data_size_t {
  static constexpr auto value = sizeof(esphome::rka_api::rka_data_t);
};

}  // namespace rka_api
}  // namespace esphome
