#include "esphome/components/vport/vport_uart.h"
#include "../components/ets/ets_component.h"
#include "../components/rka_cloud/rka_cloud.h"

#include "utils.h"

DEFINE_TAG;

using namespace esphome;
using namespace esphome::rka_api;

class ETSComponentTest : public ets::ETSComponent {
 public:
  ETSComponentTest(ets::ETSApi *api) : ets::ETSComponent(api) {}

  ets::ets_state_t state{};

  void on_state(const ets::ets_state_t &st) override {
    this->api_->init_unk0C(st.unk0C);
    state = st;
    ESP_LOGD(TAG, "got state %s", hexencode_cstr(st));
  }
};

bool test_ets() {
  bool res = true;

  auto x = cloak::from_hex("aa1588 0000 00fa 0114 0000 ffa1 0102 01c2 0000 1e00 0101 dc");
  uart::UARTComponent uart(x);
  ets::ETSUartIO io(&uart);
  ets::ETSVPort vport(&io);
  ets::ETSApi api(&vport);
  ETSComponentTest comp(&api);
  cloak::setup_and_loop({&vport, &comp});

  res &= cloak::check_data("init", uart, "AA.01 01 AC");

  res &= cloak::check_data("state", cloak::from_data(comp.state),
                           "00.00.00.FA.01.14.00.00.FF.A1.01.02.01.C2.00.00.1E.00.01.01");

  api.request_state_ex();
  vport.call_loop();
  res &= cloak::check_data("request_state_ex", uart, "AA.03 08.10.04 C9");

  ets::ets_mode_t set{};
  api.write_st(set);
  vport.call_loop();
  res &= cloak::check_data("write_st", uart, "AA.15 0A.7F.7F.00.00.00.00.00.00.00.00.7F.7F.00.00.00.7F.7F.00.7F.7F C1");

  api.set_mode(nullptr, 23, 18);
  vport.call_loop();
  res &= cloak::check_data("set_mode (set tmp)", uart,
                           "AA.15 0A.7F.7F.00.E6.00.00.00.B4.00.00.7F.7F.01.C2.00.7F.7F.00.7F.7F 1E");

  bool enabled = true;
  api.set_mode(&enabled, 23, 18);
  vport.call_loop();
  res &= cloak::check_data("set_mode (turn on)", uart,
                           "AA.15 0A.01.7F.00.E6.00.00.00.B4.00.00.7F.7F.01.C2.00.7F.7F.00.7F.7F A0");

  return res;
}

REGISTER_TEST(test_ets);
