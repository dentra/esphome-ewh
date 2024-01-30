#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "etl/delegate.h"
#include "etl/checksum.h"
#include "etl/frame_check_sequence.h"

#include "esphome/core/log.h"
#include "esphome/core/component.h"
#include "esphome/components/uart/uart_component.h"

#include "esphome/components/vport/vport_uart.h"

#include "../components/ets/ets_component.h"
#include "../components/ets/vport/ets_vport.h"
#include "../components/rka_api/rka_api.h"

#include "utils.h"

static const char *const TAG = "main";

using namespace esphome;
using namespace esphome::rka_api;

bool test_frame() {
  auto x = cloak::from_hex("aa1588 0000 00fa 0114 0000 ffa1 0102 01c2 0000 1e00 0101 dc");

  // auto f = any_frame_t::from(x.data());
  // ESP_LOGV(TAG, "frame: %s", hexencode(f, x.size()).c_str());
  // ESP_LOGD(TAG, "frame.is_valid: %s", YESNO(f->is_valid(x.size())));
  // ESP_LOGD(TAG, "frame.crc: %02x", f->crc());

  uart::UARTComponent uart(x);
  ets::ETSUartIO io(&uart);
  ets::ETSVPort vport(&io);
  ets::ETSApi p(&vport);

  // p.reader.set([](auto frame) { ESP_LOGD(TAG, "frame.data: %s", hexencode(frame->data, frame->size).c_str()); });
  io.poll();
  p.write(PACKET_REQ_STATE);
  struct {
    uint8_t x[2];
  } state_req{0x10, 0x04};
  p.write(PACKET_REQ_STATE, state_req);
  p.write(PACKET_RSP_DEV_TYPE, rka_dev_type_t{.unknown = 0, .type = rka_dev_type_t::ETS});
  ESP_LOGD(TAG, "wr: %s", uart.test_data_str().c_str());
  uart.test_data_clear();

  return true;
}

void uart_send(uart::UARTComponent *uart, uint8_t cmd, const std::vector<uint8_t> &data) {
  auto calc_crc = [](uint8_t init, const void *data, uint32_t size) -> uint8_t {
    auto data8 = static_cast<const uint8_t *>(data);
    while (size--) {
      init += *data8++;
    }
    return init;
  };
  uint8_t size = data.size() + 1;
  uint8_t crc = calc_crc(0xaa, &size, sizeof(size));
  crc = calc_crc(crc, &cmd, sizeof(cmd));
  crc = calc_crc(crc, &data[0], data.size());

  uart->write_byte(0xaa);
  uart->write_byte(size);
  uart->write_byte(cmd);
  uart->write_array(&data[0], data.size());
  uart->write_byte(crc);
  uart->flush();
}

bool test_basic() {
  // VPort<uint8_t> vport;
  // ESP_LOGD(TAG, "vport size = %d", sizeof(vport));
  // VirtVportImpl vvport;
  // ESP_LOGD(TAG, "vport size = %d", sizeof(vvport));

  ets::ets_state_t rsp{};
  rsp.target_temp_ = __builtin_bswap16(0x010e);
  rsp.air_temp_ = __builtin_bswap16(0x0114);
  rsp.unk0C = __builtin_bswap16(0x01c2);

  ets::ets_mode_t req{};
  req.target_temp(rsp.target_temp());
  req.air_temp(rsp.air_temp());
  req.unk0C = rsp.unk0C;
  req.unk0E = rsp.unk0E;

  struct {
    uint8_t magic{0xaa};
    uint8_t size{0x15};
    uint8_t cmd{0x0a};
    ets::ets_mode_t req;
    uint8_t crc;
  } PACKED full_req{.size = sizeof(req) + 1 /*cmd*/, .cmd = 0x0a, .req = req};

  printf("%s\n", format_hex_pretty(&full_req, sizeof(full_req)).c_str());

  auto x = cloak::from_hex("aa1588 0000 00fa 0114 0000 ffa1 0102 01c2 0000 1e00 0101");  // dc
  uint8_t v = 0;
  for (auto c : x) {
    v += c;
  }
  printf("%x %s\n", v, ONOFF(v == 0xDC));

  auto id = [](esphome::uart::UARTComponent &uart) -> esphome::uart::UARTComponent * { return &uart; };

  esphome::uart::UARTComponent g_uart;

  uart_send(id(g_uart), 0x88, {0x00, 0x00, 0x00, 0xfa, 0x01, 0x14, 0x00, 0x00, 0xff, 0xa1,
                               0x01, 0x02, 0x01, 0xc2, 0x00, 0x00, 0x1e, 0x00, 0x01, 0x01});
  ESP_LOGV(TAG, "uart: %s", g_uart.test_data_str().c_str());
  g_uart.test_data_clear();

  uart_send(id(g_uart), 0x88, cloak::from_hex("0000 00fa 0114 0000 ffa1 0102 01c2 0000 1e00 0101"));
  ESP_LOGV(TAG, "uart: %s", g_uart.test_data_str().c_str());
  g_uart.test_data_clear();

  uart_send(id(g_uart), 0x08, {});
  ESP_LOGV(TAG, "uart: %s", g_uart.test_data_str().c_str());
  g_uart.test_data_clear();

  return true;
}

REGISTER_TEST(test_basic);
REGISTER_TEST(test_frame);
