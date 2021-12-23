#pragma once

#ifdef EWH_CLOUD_ENABLE
#include <ESPAsyncTCP.h>
#endif

namespace esphome {
namespace ewh {

class EWHComponent;

class EWHCloud {
 public:
  explicit EWHCloud(EWHComponent *ewh) : ewh_(ewh) {}
  ~EWHCloud() { this->disconnect(); }

  void on_cloud_data(const uint8_t *data, uint32_t size);
  void on_device_frame(const uint8_t *data, uint32_t size, uint8_t crc);
  void on_cloud_frame(const PacketType type, const uint8_t *data, uint32_t size);
  void connect();
  void disconnect();
  bool is_connected();

 protected:
  AsyncClient *cloud_client_{};
  EWHComponent *ewh_;
};

}  // namespace ewh
}  // namespace esphome
