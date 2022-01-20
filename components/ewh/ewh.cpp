#include "esphome/core/log.h"
#include "esphome/core/helpers.h"

#include "ewh_utils.h"
#include "ewh.h"

namespace esphome {
namespace ewh {

static const char *const TAG = "ewh";

#define EWH_LOGV ESP_LOGD

void ElectroluxWaterHeater::read(const ewh_state_t &state) {
  auto time = this->time_->now();
  if (time.is_valid() && state.clock_hours != time.hour && state.clock_minutes != time.minute) {
    this->set_clock(time.hour, time.minute);
  }
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

void ElectroluxWaterHeater::read(const ewh_dev_type_t &dev_type) {
  if (dev_type.unknown != 0) {
    ESP_LOGW(TAG, "ewh_dev_type_t.unknown, actual %s, expected 00.00.00.00 (4)",
             format_hex_pretty(reinterpret_cast<const uint8_t *>(&dev_type.unknown), sizeof(dev_type.unknown)).c_str());
  }
  if (dev_type.type != ewh_dev_type_t::EWH) {
    ESP_LOGW(TAG, "ewh_dev_type_t.type, actual %04X, expected %04X", dev_type.type, ewh_dev_type_t::EWH);
  }
}

void ElectroluxWaterHeater::read(const ewh_result_t &result) {
  if (result.result != ewh_result_t::RESULT_OK) {
    ESP_LOGW(TAG, "ewh_result_t.result, actual %02X, expected %02X", result.result, ewh_result_t::RESULT_OK);
  }
}

void ElectroluxWaterHeater::read(const PacketType type, const void *data, uint32_t size) {
  ESP_LOGW(TAG, "Unknown packet: %02X: %s", type, format_hex_pretty(static_cast<const uint8_t *>(data), size).c_str());
}

void ElectroluxWaterHeater::exec(uint8_t cmd, const uint8_t *data, uint32_t size) {
  ESP_LOGD(TAG, "Exec %02X: %s", cmd, format_hex_pretty(data, size).c_str());
  this->send_packet_(static_cast<PacketType>(cmd), data, size);
}

void ElectroluxWaterHeater::exec(uint8_t cmd, const std::vector<uint8_t> &data) {
  this->exec(cmd, data.data(), data.size());
}

void ElectroluxWaterHeater::exec(const std::string &datastr) {
  std::vector<uint8> data = from_hex(datastr);
  if (data.empty()) {
    ESP_LOGW(TAG, "Exec empty or invalid data %s", datastr.c_str());
    return;
  }
  this->exec(*data.data(), data.data() + 1, data.size() - 1);
}

void ElectroluxWaterHeater::set_mode(ewh_mode_t::Mode mode, uint8_t target_temperature) {
  this->set_mode(ewh_mode_t{
      .mode = mode,
      .temperature = target_temperature,
  });
}

void ElectroluxWaterHeater::set_clock(uint8_t hours, uint8_t minutes) {
  this->set_clock(ewh_clock_t{
      .hours = hours,
      .minutes = minutes,
  });
}

void ElectroluxWaterHeater::set_timer(uint8_t hours, uint8_t minutes, uint8_t temperature, ewh_mode_t::Mode mode) {
  this->set_timer(ewh_timer_t{
      .hours = hours,
      .minutes = minutes,
      .mode = mode,
      .temperature = temperature,
  });
}

void ElectroluxWaterHeater::set_bst(bool value) {
  this->set_bst(ewh_bst_t{
      .state = value ? ewh_bst_t::STATE_ON : ewh_bst_t::STATE_OFF,
  });
}

void ElectroluxWaterHeater::send_control_state_(uint8_t operation, const void *data, int size) {
  EWH_LOGV(TAG, "Change control state %02X: %s", operation,
           format_hex_pretty(static_cast<const uint8_t *>(data), size).c_str());

  const uint32_t buf_size = size + sizeof(uint8_t);
  uint8_t *buf = new uint8_t[buf_size];
  *buf = operation;
  std::memcpy(buf + sizeof(uint8_t), data, size);
  this->send_packet_(PACKET_REQ_SET_COMMAND, buf, buf_size);
  delete[] buf;

  // execute request state on next loop cycle
  this->defer(TAG, [this]() { this->request_state(); });
}

void ElectroluxWaterHeater::request_dev_type() {
  ESP_LOGD(TAG, "Request dev type");
  this->send_packet_(PACKET_REQ_DEV_TYPE, nullptr, 0);
}

void ElectroluxWaterHeater::request_state() {
  ESP_LOGD(TAG, "Request state");
  static const uint8_t req[] = {0x10, 0x04};
  this->send_packet_(PACKET_REQ_STATE, req, sizeof(req));
}

void ElectroluxWaterHeater::set_mode(const ewh_mode_t &mode) {
  ESP_LOGD(TAG, "Set mode to %u, temperature %u", mode.mode, mode.temperature);
  this->send_control_state_(ewh_mode_t::SET_OPERATION, &mode, sizeof(mode));
}

void ElectroluxWaterHeater::set_timer(const ewh_timer_t &timer) {
  if (timer.hours > 23) {
    ESP_LOGW(TAG, "Timer hours must be in [0:23] range");
    return;
  }
  if (timer.minutes > 59) {
    ESP_LOGW(TAG, "Timer minutes must be in [0:59] range");
    return;
  }
  if (timer.temperature < MIN_TEMPERATURE || timer.temperature > MAX_TEMPERATURE) {
    ESP_LOGW(TAG, "Timer temperature must be in [%u:%u] range", MIN_TEMPERATURE, MAX_TEMPERATURE);
    return;
  }
  ESP_LOGD(TAG, "Set timer to %02u:%02u, mode %u, temperature %u", timer.hours, timer.minutes, timer.mode,
           timer.temperature);
  this->send_control_state_(ewh_timer_t::SET_OPERATION, &timer, sizeof(timer));
}

void ElectroluxWaterHeater::set_clock(const ewh_clock_t &clock) {
  if (clock.hours > 23) {
    ESP_LOGW(TAG, "Clock hours must be in [0:23] range");
    return;
  }
  if (clock.minutes > 59) {
    ESP_LOGW(TAG, "Clock minutes must be in [0:59] range");
    return;
  }
  ESP_LOGD(TAG, "Set clock to %02u:%02u", clock.hours, clock.minutes);
  this->send_control_state_(ewh_clock_t::SET_OPERATION, &clock, sizeof(clock));
}

void ElectroluxWaterHeater::set_bst(const ewh_bst_t &bst) {
  ESP_LOGD(TAG, "Set BST to %s", ONOFF(bst.state == ewh_bst_t::STATE_ON));
  this->send_control_state_(ewh_bst_t::SET_OPERATION, &bst, sizeof(bst));
}

// send packet. data must be without prefix (AA), size and crc
void ElectroluxWaterHeater::send_packet_(const PacketType packet_req_type, const uint8_t *data, const uint32_t size) {
  const uint32_t packet_size = size + sizeof(PacketType);
  uint8_t crc = ewh::crc8(FRAME_HEADER, packet_size, packet_req_type, data, size);
  EWH_LOGV(TAG, "Send %02X: %s CRC: %02X", packet_req_type, format_hex_pretty(data, size).c_str(), crc);
  this->uart_->write_byte(FRAME_HEADER);
  this->uart_->write_byte(packet_size);
  this->uart_->write_byte(packet_req_type);
  if (size > 0) {
    this->uart_->write_array(data, size);
  }
  this->uart_->write_byte(crc);
  this->uart_->flush();
}

bool ElectroluxWaterHeater::read_packet_check_(const uint32_t actual, const uint32_t expected) const {
  if (expected == actual) {
    return true;
  }
  ESP_LOGW(TAG, "Invalid packet size, expected: %02X, actual: %02X", expected, actual);
  return false;
}

void ElectroluxWaterHeater::read_packet_(const PacketType packet_rsp_type, const void *data, uint32_t size) {
  if (packet_rsp_type == PACKET_STATUS) {
    if (this->read_packet_check_(size, sizeof(ewh_status_t))) {
      for (auto listener : this->listeners_) {
        listener->read(*static_cast<const ewh_status_t *>(data));
      }
    }
  } else if (packet_rsp_type == PACKET_RSP_STATE) {
    if (this->read_packet_check_(size, sizeof(ewh_state_t))) {
      for (auto listener : this->listeners_) {
        listener->read(*static_cast<const ewh_state_t *>(data));
      }
    }
  } else if (packet_rsp_type == PACKET_RSP_DEV_TYPE) {
    if (this->read_packet_check_(size, sizeof(ewh_dev_type_t))) {
      for (auto listener : this->listeners_) {
        listener->read(*static_cast<const ewh_dev_type_t *>(data));
      }
    }
  } else if (packet_rsp_type == PACKET_RSP_SAVE_DATA) {
    if (this->read_packet_check_(size, sizeof(ewh_result_t))) {
      for (auto listener : this->listeners_) {
        listener->read(*static_cast<const ewh_result_t *>(data));
      }
    }
  } else if (packet_rsp_type == PACKET_RSP_LOAD_DATA) {
    if (this->read_packet_check_(size, sizeof(ewh_data_t))) {
      for (auto listener : this->listeners_) {
        listener->read(*static_cast<const ewh_data_t *>(data));
      }
    }
  } else if (packet_rsp_type == PACKET_ERROR) {
    if (this->read_packet_check_(size, sizeof(ewh_error_t))) {
      for (auto listener : this->listeners_) {
        listener->read(*static_cast<const ewh_error_t *>(data));
      }
    }
  } else {
    for (auto listener : this->listeners_) {
      listener->read(packet_rsp_type, static_cast<const uint8_t *>(data), size);
    }
  }
}

void ElectroluxWaterHeater::read_frame_(const uint8_t *data, uint32_t size, uint8_t crc) {
  EWH_LOGV(TAG, "Read %02X: %s CRC: %02X", *data, format_hex_pretty(data + 1, size - 1).c_str(), crc);
  uint8_t calc_crc = ewh::crc8(FRAME_HEADER, size, data, size);
  if (crc != calc_crc) {
    ESP_LOGW(TAG, "Invalid frame checksum, expected: %02X, actual: %02X", crc, calc_crc);
    return;
  }
  this->read_packet_(static_cast<PacketType>(*data), data + 1, size - 1);
}

void ElectroluxWaterHeater::read_data_() {
  while (this->uart_->available()) {
    uint8_t ch;
    if (!this->uart_->read_byte(&ch)) {
      break;
    }

    if (this->await_size_ > 0) {
      ESP_LOGVV(TAG, "Await buf %02X, buf_size=%u, await_size=%d", ch, this->buf_size_, this->await_size_);
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
      ESP_LOGVV(TAG, "Await size %02X (%d)", ch, ch);
      this->await_size_ = ch;
      if (this->await_size_ > sizeof(this->buf_)) {
        ESP_LOGW(TAG, "Answer is to large: %d", ch);
        this->await_size_ = sizeof(this->buf_);
      }
      continue;
    }

    // await_size == 0
    if (ch == FRAME_HEADER) {
      ESP_LOGVV(TAG, "Await magic %02X", ch);
      this->await_size_ = -1;
      continue;
    }

    ESP_LOGW(TAG, "Not expected byte 0x%02X", ch);
  }
}

}  // namespace ewh
}  // namespace esphome
