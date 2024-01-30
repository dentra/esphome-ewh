#include "esphome/components/vport/vport_uart.h"
#include "../components/bwh/bwh_component.h"

#include "utils.h"

using namespace esphome;

DEFINE_TAG;

class BWHComponentTest : public bwh::BWHComponent {
 public:
  BWHComponentTest(bwh::BWHApi *api) : bwh::BWHComponent(api) {}

  bwh::bwh_state_t state{};

  void on_state(const bwh::bwh_state_t &st) override {
    state = st;
    ESP_LOGD(TAG, "got state %s", hexencode_cstr(st));
  }
};

bool test_bwh() {
  bool res = true;

  auto x = cloak::from_hex("aa 09 88 02.10.11.00.00.04.00.00 62");
  uart::UARTComponent uart(x);
  bwh::BWHUartIO io(&uart);
  bwh::BWHVPort vport(&io);
  bwh::BWHApi api(&vport);
  BWHComponentTest comp(&api);
  cloak::setup_and_loop({&vport, &comp});

  res &= cloak::check_data("init", uart, "AA.01 01 AC");

  res &= cloak::check_data("state", cloak::from_data(comp.state), "02.10.11.00.00.04.00.00");

  api.request_state();
  vport.call_loop();
  res &= cloak::check_data("request_state", uart, "AA.01 08 B3");

  api.request_state_ex();
  vport.call_loop();
  res &= cloak::check_data("request_state_ex", uart, "AA.03 08.10.04 C9");

  api.set_mode(bwh::bwh_mode_t::MODE_2000W, 23);
  vport.call_loop();
  res &= cloak::check_data("set_mode", uart, "AA.04 0A.00.02.17 D1");

  return res;
}

REGISTER_TEST(test_bwh);
