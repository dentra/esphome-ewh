#include "esphome/components/vport/vport_uart.h"
#include "../components/ets/ets_component.h"
#include "../components/ets/vport/ets_vport.h"
#include "../components/rka_cloud/rka_cloud.h"
#include "../components/rka_cloud/rka_pair.h"

#include "utils.h"

DEFINE_TAG;

using namespace esphome;
using namespace esphome::rka_api;

class RKACloudTest : public rka_cloud::RKACloud {
  class RKACloudPairTest : public rka_cloud::RKACloudPair {
   public:
    RKACloudPairTest(RKACloudTest *cloud) : rka_cloud::RKACloudPair(cloud) {}
    AsyncUDP *udp() { return this->udp_ == nullptr ? nullptr : this->udp_->client(); }
  };

 public:
  RKACloudTest(RKAVPort *vport) : rka_cloud::RKACloud(vport) { this->set_cloud_pair(new RKACloudPairTest(this)); }
  virtual ~RKACloudTest() { delete this->cloud_pair_; }

  AsyncClient *tcp() { return this->tcp_->client(); }
  AsyncUDP *udp() { return this->pair()->udp(); }
  void pair(bool enable) { this->pair()->pair(enable); }
  RKACloudPairTest *pair() { return reinterpret_cast<RKACloudPairTest *>(this->cloud_pair_); }
};

bool test_tcp(RKACloudTest &cloud, uart::UARTComponent &uart, ets::ETSVPort &vport) {
  bool res = true;

  cloud.connect();

  auto t = cloak::from_hex("aa1588 0000 00fa 0114 0000 ffa1 0102 01c2 0000 1e00 0101 dc");
  auto x = cloak::from_hex("88 0000 00fa 0114 0000 ffa1 0102 01c2 0000 1e00 0101");

  cloud.on_frame(*reinterpret_cast<const rka_any_frame_t *>(x.data()), x.size());
  res &= cloak::check_data("on_frame", cloud.tcp(), t);

  // single AA
  cloud.tcp()->test_data_push(t.data(), t.size());
  vport.call_loop();
  res &= cloak::check_data("push AA", &uart, t);
  res &= cloak::check_data("push AA", cloud.tcp(), t);

  // single AT
  auto at_cmd = "AT+WSMAC\r\n\r\n";
  cloud.tcp()->test_data_push(at_cmd);
  vport.call_loop();
  auto rsp_str = "+ok=112233445566\r\n\r\n";
  auto rsp_arr = std::vector<uint8_t>(rsp_str, rsp_str + strlen(rsp_str));
  res &= cloak::check_data("push AT", cloud.tcp(), rsp_arr);

  // multiple AA
  t = cloak::from_hex("AA 03 08 10 04 C9 AA 01 07 B2 AA 01 01 AC");
  cloud.tcp()->test_data_push(t.data(), t.size());
  vport.call_loop();
  vport.call_loop();
  vport.call_loop();
  res &= cloak::check_data("push multiple AA", &uart, t);

  // add AT to end of multiple AA
  x = t;
  x.insert(x.end(), at_cmd, at_cmd + strlen(at_cmd));
  cloud.tcp()->test_data_push(x.data(), x.size());
  vport.call_loop();
  vport.call_loop();
  vport.call_loop();
  res &= cloak::check_data("push multiple AA + AT (AT)", cloud.tcp(), rsp_arr);
  res &= cloak::check_data("push multiple AA + AT (AA)", &uart, t);

  // add AT to begin of multiple AA
  x = t;
  x.insert(x.begin(), at_cmd, at_cmd + strlen(at_cmd));
  cloud.tcp()->test_data_push(x.data(), x.size());
  vport.call_loop();
  vport.call_loop();
  vport.call_loop();
  res &= cloak::check_data("push multiple AA + AT (AT)", cloud.tcp(), rsp_arr);
  res &= cloak::check_data("push multiple AA + AT (AA)", &uart, t);

  t = cloak::from_hex("AA 03 08 10 04 C9 AA 01 07 B2");
  x = t;
  x.insert(x.end(), at_cmd, at_cmd + strlen(at_cmd));
  auto tail = cloak::from_hex("AA 01 01 AC");
  t.insert(t.end(), tail.begin(), tail.end());
  x.insert(x.end(), tail.begin(), tail.end());
  cloud.tcp()->test_data_push(x.data(), x.size());
  vport.call_loop();
  vport.call_loop();
  vport.call_loop();
  res &= cloak::check_data("push multiple AA + AT + AA (AT)", cloud.tcp(), rsp_arr);
  res &= cloak::check_data("push multiple AA + AT + AA (AA)", &uart, t);

  at_cmd = "AT+UNKNOWN\r\n\r\n\r\n";
  t = cloak::from_hex("AA 03 08 10 04 C9 AA 01 07 B2");
  x = t;
  x.insert(x.end(), at_cmd, at_cmd + strlen(at_cmd));
  tail = cloak::from_hex("AA 01 01 AC");
  t.insert(t.end(), tail.begin(), tail.end());
  x.insert(x.end(), tail.begin(), tail.end());
  cloud.tcp()->test_data_push(x.data(), x.size());
  vport.call_loop();
  vport.call_loop();
  vport.call_loop();
  res &= cloak::check_data("push multiple AA + AT + AA (AT)", cloud.tcp(), cloak::from_hex(""));
  res &= cloak::check_data("push multiple AA + AT + AA (AA)", &uart, t);

  uint8_t uid[4]{0, 26, 37, 4};  // 263704 -> 1a.25.04
  cloud.set_uid(uid);
  cloud.dump_config();
  cloud.tcp()->test_data_push(cloak::from_hex("AA 01 07 B2"));
  vport.call_loop();
  res &= cloak::check_data("configured UID", cloud.tcp(),
                           cloak::from_hex("AA.11.87.00.00.00.00.00.00.00.00.00.00.00.00.00.1A.25.04.85"));

  cloud.disconnect();
  return res;
}

bool test_udp(RKACloudTest &cloud, ets::ETSVPort &vport) {
  bool res = true;
  cloud.pair(true);
  cloud.pair()->test_timeout(true);
  cloud.udp()->test_data_push("smartlinkfind");
  auto rsp_str = "smart_config 112233445566";
  res &= cloak::check_data("push pair", cloud.udp(), std::vector<uint8_t>(rsp_str, rsp_str + strlen(rsp_str)));
  cloud.pair()->test_timeout(false);
  res &= cloak::check_data("udp free", cloud.udp() == nullptr, true);
  cloud.pair(false);
  return res;
}

bool test_cloud() {
  bool res = true;

  uart::UARTComponent uart(cloak::from_hex("aa1588 0000 00fa 0114 0000 ffa1 0102 01c2 0000 1e00 0101 dc"));
  ets::ETSUartIO io(&uart);
  ets::ETSVPort vport(&io);

  RKACloudTest cloud(&vport);
  cloud.set_host("test.com");
  cloud.set_port(1979);
  uint8_t mac[6] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
  cloud.set_mac(mac);

  res &= test_tcp(cloud, uart, vport);
  res &= test_udp(cloud, vport);

  return res;
}

REGISTER_TEST(test_cloud);
