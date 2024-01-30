#include "esphome/components/vport/vport_uart.h"
#include "../components/ewh/ewh_component.h"

#include "utils.h"

using namespace esphome;

DEFINE_TAG;

class EWHComponentTest : public ewh::EWHComponent {
 public:
  EWHComponentTest(ewh::EWHApi *api) : ewh::EWHComponent(api) {}

  ewh::ewh_state_t state{};

  void on_state(const ewh::ewh_state_t &st) override {
    state = st;
    ESP_LOGD(TAG, "got state %s", hexencode_cstr(st));
  }
};

bool test_ewh() {
  bool res = true;

  auto x = cloak::from_hex("aa 0a 09 03.10.11.00.00.00.00.00.00 e1");
  uart::UARTComponent uart(x);
  ewh::EWHUartIO io(&uart);
  ewh::EWHVPort vport(&io);
  ewh::EWHApi api(&vport);
  EWHComponentTest comp(&api);
  cloak::setup_and_loop({&vport, &comp});

  res &= cloak::check_data("init", uart, "AA.01 01 AC");

  res &= cloak::check_data("state", cloak::from_data(comp.state), "03.10.11.00.00.00.00.00.00");

  api.request_state();
  vport.call_loop();
  res &= cloak::check_data("request_state", uart, "AA.01 08 B3");

  api.request_state_ex();
  vport.call_loop();
  res &= cloak::check_data("request_state_ex", uart, "AA.03 08.10.04 C9");

  api.set_mode(ewh::ewh_mode_t::MODE_2000W, 23);
  vport.call_loop();
  res &= cloak::check_data("set_mode", uart, "AA.04 0A.00.03.17 D2");

  api.set_bst(true);
  vport.call_loop();
  res &= cloak::check_data("set_bst", uart, "AA.03 0A.03.01 BB");

  api.set_clock(14, 45);
  vport.call_loop();
  res &= cloak::check_data("set_clock", uart, "AA.04 0A.01.0E.2D F4");

  api.set_timer(14, 42, 44, ewh::ewh_mode_t::MODE_700W);
  vport.call_loop();
  res &= cloak::check_data("set_timer", uart, "AA.06 0A.02.0E.2A.01.2C 21");

  return res;
}

bool test_ewh_rsp_8a() {
  bool res = true;

  auto x = cloak::from_hex("AA 0A 8A 01 14 00 00 00 00 00 00 01 54");
  uart::UARTComponent uart(x);
  ewh::EWHUartIO io(&uart);
  ewh::EWHVPort vport(&io);
  ewh::EWHApi api(&vport);
  EWHComponentTest comp(&api);
  cloak::setup_and_loop({&vport, &comp});

  for (int i = 0; i < 100; i++) {
    vport.loop();
    comp.loop();
  }

  res &= cloak::check_data("state", cloak::from_data(comp.state), "03.10.11.00.00.00.00.00.00");

  return res;
}

REGISTER_TEST(test_ewh);
REGISTER_TEST(test_ewh_rsp_8a);
