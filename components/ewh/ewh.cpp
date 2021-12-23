#include "esphome/core/log.h"
#include "esphome/core/helpers.h"

#include "ewh.h"

namespace esphome {
namespace ewh {

static const char *const TAG = "ewh";

void ElectroluxWaterHeater::send_control_state_(uint8_t operation, const void *data, int size) {
  ESP_LOGV(TAG, "Change control state %02X: %s", operation,
           hexencode(static_cast<const uint8_t *>(data), size).c_str());

  int buf_size = size + 1;
  uint8_t *buf = new uint8_t[buf_size];
  *buf = operation;
  std::memcpy(buf + 1, data, size);
  this->send_packet_(PACKET_REQ_SET_COMMAND, buf, buf_size);
  delete[] buf;

  this->send_request_state_();
}

void ElectroluxWaterHeater::send_request_state_() {
  ESP_LOGD(TAG, "Request state");
  static const uint8_t req[] = {0x10, 0x04};
  this->send_packet_(PACKET_REQ_STATE, req, sizeof(req));
}

void ElectroluxWaterHeater::send_set_mode(const ewh_mode_t &mode) {
  ESP_LOGD(TAG, "Set mode to %u, temperature %u", mode.mode, mode.temperature);
  this->send_control_state_(ewh_mode_t::SET_OPERATION, &mode, sizeof(mode));
}

void ElectroluxWaterHeater::send_set_timer(const ewh_timer_t &timer) {
  ESP_LOGD(TAG, "Set timer to %02d:%02d, mode %d, temperature %d", timer.hours, timer.minutes, timer.mode,
           timer.temperature);
  this->send_control_state_(ewh_timer_t::SET_OPERATION, &timer, sizeof(timer));
}

void ElectroluxWaterHeater::send_set_timer(uint8_t hours, uint8_t minutes, uint8_t mode, uint8_t temperature) {
  const ewh_timer_t timer = {
      .hours = hours,
      .minutes = minutes,
      .mode = mode,
      .temperature = temperature,
  };
  this->send_set_timer(timer);
}

void ElectroluxWaterHeater::send_set_clock(const ewh_clock_t &clock) {
  ESP_LOGD(TAG, "Set clock to %02d:%02d", clock.hours, clock.minutes);
  this->send_control_state_(ewh_clock_t::SET_OPERATION, &clock, sizeof(clock));
}

void ElectroluxWaterHeater::send_set_clock(uint8_t hours, uint8_t minutes) {
  const ewh_clock_t clock = {.hours = hours, .minutes = minutes};
  this->send_set_clock(clock);
}

void ElectroluxWaterHeater::send_set_bst(const ewh_bst_t &bst) {
  ESP_LOGD(TAG, "Set BST to %s", ONOFF(bst.state == ewh_bst_t::STATE_ON));
  this->send_control_state_(ewh_bst_t::SET_OPERATION, &bst, sizeof(bst));
}

void ElectroluxWaterHeater::send_set_bst(bool value) {
  const ewh_bst_t bst = {.state = value ? ewh_bst_t::STATE_ON : ewh_bst_t::STATE_OFF};
  this->send_set_bst(bst);
}

// send packet. data must be without prefix (AA), size and crc
void ElectroluxWaterHeater::send_packet_(const PacketType packet_req_type, const uint8_t *data, const uint32_t size) {
  const uint32_t packet_size = size + sizeof(PacketType);
  uint8_t crc = this->crc8_(FRAME_HEADER, packet_size, packet_req_type, data, size);
  ESP_LOGV(TAG, "Send %02X: %s CRC: %02X", packet_req_type, hexencode(data, size).c_str(), crc);
  this->write_byte(FRAME_HEADER);
  this->write_byte(packet_size);
  this->write_byte(packet_req_type);
  this->write_array(data, size);
  this->write_byte(crc);
  this->flush();
}

void ElectroluxWaterHeater::read(const ewh_error_t &error) {
  switch (error.code) {
    case ewh_error_t::CODE_BAD_CRC:
      ESP_LOGW(TAG, "Operation failed, invalid CRC");
      break;
    case ewh_error_t::CODE_BAD_COMMAND:
      ESP_LOGW(TAG, "Operation failed, invalid command");
      break;
    default:
      ESP_LOGW(TAG, "Operation failed, code %02X", error.code);
      break;
  }
}

bool ElectroluxWaterHeater::read(const ewh_unknown81 &unk) {
  bool res = true;
  if (unk.unknown11 != 0x11) {
    ESP_LOGW(TAG, "ewh_unknown81.unknown11, actual %02X, expected 11", unk.unknown11);
    res = false;
  }
  return res;
}

bool ElectroluxWaterHeater::read(const ewh_unknown86 &unk) {
  bool res = true;
  if (unk.unknown01 != 0x01) {
    ESP_LOGW(TAG, "ewh_unknown86.unknown01, actual %02X, expected 01", unk.unknown01);
    res = false;
  }
  return res;
}

bool ElectroluxWaterHeater::read(const ewh_unknown87 &unk) {
  bool res = true;
  for (auto v : unk.unknown00_8) {
    if (v != 0x00) {
      ESP_LOGW(TAG, "ewh_unknown87.unknown00_8, actual %s, expected 00.00.00.00.00.00.00.00 (8)",
               hexencode(unk.unknown00_8, sizeof(unk.unknown00_8)).c_str());
      res = false;
      break;
    }
  }

  if (unk.unknownB9 != 0x00 && unk.unknownB9 != 0xB9) {
    ESP_LOGW(TAG, "ewh_unknown87.unknownB9, actual %02X, expected 00 or B9", unk.unknownB9);
    res = false;
  }

  for (auto v : unk.unknown00_4) {
    if (v != 0x00) {
      ESP_LOGW(TAG, "ewh_unknown87.unknown00_4, actual %s, expected 00.00.00.00 (4)",
               hexencode(unk.unknown00_4, sizeof(unk.unknown00_4)).c_str());
      res = false;
      break;
    }
  }

  if (unk.unknown1A != 0x00 && unk.unknown1A != 0x1A) {
    ESP_LOGW(TAG, "ewh_unknown87.unknown1A, actual %02X, expected 00 or A1", unk.unknown1A);
    res = false;
  }

  if (unk.unknown07 != 0x00 && unk.unknown07 != 0x07) {
    ESP_LOGW(TAG, "ewh_unknown87.unknown07, actual %02X, expected 00 or 07", unk.unknown07);
    res = false;
  }

  if (unk.unknown32 != 0x00 && unk.unknown32 != 0x32) {
    ESP_LOGW(TAG, "ewh_unknown87.unknown32, actual %02X, expected 00 or 32", unk.unknown32);
    res = false;
  }

  return res;
}

bool ElectroluxWaterHeater::read_packet_check_(const uint32_t actual, const uint32_t expected) const {
  if (expected == actual) {
    return true;
  }
  ESP_LOGW(TAG, "Invalid packet size, expected: %02X, actual: %02X", expected, actual);
  return false;
}

void ElectroluxWaterHeater::read_packet_(const PacketType packet_rsp_type, const void *data, uint32_t size) {
  if (packet_rsp_type == PACKET_RSP_STATE || packet_rsp_type == PACKET_RSP_STATUS) {
    if (this->read_packet_check_(size, sizeof(ewh_state_t))) {
      this->read(*static_cast<const ewh_state_t *>(data));
    }
  } else if (packet_rsp_type == PACKET_RSP_ERROR) {
    if (this->read_packet_check_(size, sizeof(ewh_error_t))) {
      this->read(*static_cast<const ewh_error_t *>(data));
    }
  } else if (packet_rsp_type == PACKET_RSP_UNKNOWN81) {
    if (this->read_packet_check_(size, sizeof(ewh_unknown81))) {
      this->read(*static_cast<const ewh_unknown81 *>(data));
    }
  } else if (packet_rsp_type == PACKET_RSP_UNKNOWN86) {
    if (this->read_packet_check_(size, sizeof(ewh_unknown86))) {
      this->read(*static_cast<const ewh_unknown86 *>(data));
    }
  } else if (packet_rsp_type == PACKET_RSP_UNKNOWN87) {
    if (this->read_packet_check_(size, sizeof(ewh_unknown87))) {
      this->read(*static_cast<const ewh_unknown87 *>(data));
    }
  } else {
    ESP_LOGW(TAG, "Unknown packet: %02X", packet_rsp_type);
    this->read(packet_rsp_type, static_cast<const uint8_t *>(data), size);
  }
}

void ElectroluxWaterHeater::read_frame_(const uint8_t *data, uint32_t size, uint8_t crc) {
  ESP_LOGV(TAG, "Read %02X: %s CRC: %02X", *data, hexencode(data + 1, size - 1).c_str(), crc);
  uint8_t calc_crc = this->crc8_(FRAME_HEADER, size, data, size);
  if (crc != calc_crc) {
    ESP_LOGW(TAG, "Invalid packet crc, expected: %02X, actual: %02X", crc, calc_crc);
    return;
  }
  this->read_packet_(static_cast<PacketType>(*data), data + 1, size - 1);
}

void ElectroluxWaterHeater::read_data_() {
  while (this->available()) {
    uint8_t ch;
    if (!this->read_byte(&ch)) {
      break;
    }

    if (this->await_size_ > 0) {
      // ESP_LOGVV(TAG, "await buf %02X, buf_size=%d, await_size=%d", ch, buf_size, await_size);
      if (this->buf_size_ < this->await_size_) {
        this->buf_[this->buf_size_++] = ch;
      } else {
        this->read_frame_(this->buf_, this->await_size_, ch);
        this->buf_size_ = 0;
        this->await_size_ = 0;
      }
      continue;
    }

    if (this->await_size_ < 0) {
      // ESP_LOGVV(TAG, "await size %02X (%d)", ch, ch);
      this->await_size_ = ch;
      if (this->await_size_ > sizeof(this->buf_)) {
        ESP_LOGW(TAG, "Answer is to large: %d", ch);
        this->await_size_ = sizeof(this->buf_);
      }
      continue;
    }

    // await_size == 0
    if (ch == FRAME_HEADER) {
      // ESP_LOGVV(TAG, "await magic %02X", ch);
      this->await_size_ = -1;
      continue;
    }

    ESP_LOGW(TAG, "Not expected byte 0x%02X", ch);
  }
}

}  // namespace ewh
}  // namespace esphome
