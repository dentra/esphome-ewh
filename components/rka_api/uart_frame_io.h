#pragma once

#include "etl/frame_check_sequence.h"
#include "etl/checksum.h"
#include "etl/delegate.h"

#include "esphome/core/log.h"
#include "esphome/core/helpers.h"
#include "esphome/core/hal.h"

#ifndef RKA_DUMP
#define RKA_DUMP ESP_LOGV
#ifdef ESPHOME_LOG_HAS_VERBOSE
#define RKA_DO_DUMP_TX 1
#endif
#else
#define RKA_DO_DUMP_TX 1
#endif

namespace esphome {
namespace rka_api {

// template<class U> class UartReader : public U {
//  public:
//   int available() { return U::available(); }
//   bool read_array(uint8_t *data, size_t size) { return U::read_array(data, size); };
//   void write_array(const uint8_t *data, size_t size) { U::write_array(data, size); };
//   void flush() { U::flush(); };
// };

// max_frame_size_value - size of frame data without magic, size and crc
template<size_t max_frame_size_value, uint8_t magic_value = 0xAA, typename size_type = uint8_t,
         typename checksum_policy = etl::checksum_policy_sum<size_type>>
class UartFrameIO {
  static_assert(std::is_integral<size_type>::value);
  static_assert(std::is_class<checksum_policy>::value);

  using magic_type = uint8_t;
  using crc_type = typename checksum_policy::value_type;

  constexpr static const char *const TAG = "uart_frame_io";

 public:
  struct rx_frame_hdr_t {
    magic_type magic;
    size_type size;
  } PACKED;
  struct rx_frame_t : rx_frame_hdr_t {
    uint8_t data[sizeof(crc_type)];
    crc_type crc() const { return *reinterpret_cast<const crc_type *>(&this->data[this->size]); }
    /// Check frame is valid with magic and whole frame size (magic + size + data + CRC).
    bool is_valid() const { return this->magic == magic_value; }
    bool is_valid_size(size_t whole_frame_size) const {
      return whole_frame_size - this->size - sizeof(rx_frame_t) == 0;
    }
  } PACKED;

  template<class U> void read(U *uart) {
    auto frame = this->rx_.frame();
    while (uart->available()) {
      if (frame->magic == 0) {
        // read magic
        if (uart->read_array(&frame->magic, sizeof(magic_type))) {
          ESP_LOGVV(TAG, "Read magic: 0x%X", frame->magic);
          if (frame->magic != magic_value) {
            ESP_LOGW(TAG, "Not expected magic: 0x%X", frame->magic);
            frame->magic = 0;
          }
        }
        continue;
      }
      if (frame->size == 0) {
        // read size
        if (uart->read_array(&frame->size, sizeof(size_type))) {
          ESP_LOGVV(TAG, "Read size: %zu", static_cast<size_t>(frame->size));
          if (frame->size > max_frame_size_value) {
            ESP_LOGW(TAG, "Frame size is larger than data buffer: %zu > %zu", static_cast<size_t>(frame->size),
                     max_frame_size_value);
            this->rx_.reset();
          }
        }
        continue;
      }
      if (this->rx_.size < frame->size) {
        // read data
        if (uart->read_array(&frame->data[this->rx_.size], sizeof(uint8_t))) {
          ESP_LOGVV(TAG, "Read data[%02u]: 0x%02X", this->rx_.size, frame->data[this->rx_.size] & 0xFF);
          this->rx_.size++;
        }
        continue;
      }
      if (this->rx_.size == frame->size) {
        // read crc
        if (uart->read_array(&frame->data[this->rx_.size], sizeof(crc_type))) {
          ESP_LOGVV(TAG, "Read CRC: 0x%02X", frame->data[this->rx_.size] & 0xFF);
          if (this->check_crc(frame)) {
            RKA_DUMP(TAG, "RX: %s", format_hex_pretty(this->rx_.data, this->rx_.size + sizeof(rx_frame_t)).c_str());
            if (this->reader_) {
              this->reader_(frame->data, frame->size);
            }
          } else {
            ESP_LOGW(TAG, "Invalid CRC for frame %s, expected",
                     format_hex_pretty(this->rx_.data, this->rx_.size + sizeof(rx_frame_t)).c_str());
          }
          this->rx_.reset();
        }

        esphome::yield();
        // let perform read next frame on next loop
        break;
      }
      ESP_LOGW(TAG, "Unhandled read operation");
      this->rx_.reset();
    }
  }

  template<class U, class T> void write(U *uart, const T &data) {
    this->write(uart, reinterpret_cast<const uint8_t *>(&data), sizeof(data));
  }

  template<class U> void write(U *uart, const uint8_t *data, size_t size) {
    rx_frame_hdr_t hdr{.magic = magic_value, .size = static_cast<size_type>(size)};

    etl::frame_check_sequence<checksum_policy> crc_seq;
    crc_seq.add(reinterpret_cast<uint8_t *>(&hdr), reinterpret_cast<uint8_t *>(&hdr) + sizeof(hdr));
    uart->write_array(reinterpret_cast<uint8_t *>(&hdr), sizeof(hdr));
    if (size != 0) {
      crc_seq.add(data, data + size);
      uart->write_array(data, size);
    }
    crc_type crc = crc_seq.value();
    uart->write_array(reinterpret_cast<uint8_t *>(&crc), sizeof(crc));
    uart->flush();
#if RKA_DO_DUMP_TX
    std::string s = format_hex_pretty(reinterpret_cast<uint8_t *>(&hdr), sizeof(hdr));
    if (size != 0) {
      s += format_hex_pretty(data, size).c_str();
      auto pos = s.find('(');
      if (pos != std::string::npos) {
        s.resize(pos - 1);
      }
    }
    s += ' ';
    s += format_hex_pretty(reinterpret_cast<uint8_t *>(&crc), sizeof(crc));
    RKA_DUMP(TAG, "TX: %s (%zu)", s.c_str(), sizeof(rx_frame_t) + size);
#endif
  }

  using reader_type = etl::delegate<void(const void *data, size_t size)>;
  void set_reader(reader_type &&reader) { this->reader_ = std::move(reader); }
  reader_type reader_;

  static crc_type calc_crc(const void *data, size_t size) {
    auto data8 = static_cast<const uint8_t *>(data);
    return etl::frame_check_sequence<checksum_policy>(data8, data8 + size).value();
  }

  etl::delegate<bool(const rx_frame_t *frame)> check_crc{[](const rx_frame_t *frame) -> bool {
    return calc_crc(frame, frame->size + sizeof(rx_frame_hdr_t)) == frame->crc();
  }};

 protected:
  struct {
    size_type size;
    uint8_t data[max_frame_size_value + sizeof(rx_frame_t)];
    rx_frame_t *frame() { return reinterpret_cast<rx_frame_t *>(this->data); }
    void reset() {
      std::memset(this->data, 0, sizeof(this->data));
      this->size = 0;
    }
  } rx_{};
};

}  // namespace rka_api
}  // namespace esphome
