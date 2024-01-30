#pragma once
#include "../rka_api/rka_data.h"

namespace esphome {
namespace ets {

#pragma pack(push, 1)

enum ets_ctl_type_t : uint8_t {
  // 00 - датчик пола
  CT_FLOOR = 0,
  // 01 - датчик воздуха
  CT_AIR = 1,
  // 02 - датчик пола и воздуха
  CT_FLOOR_AIR = 2,
};

enum ets_sens_type_t : uint8_t {
  // Caleo 5 kΩ
  ST_CALEO_5KOHM = 0,
  // Teplolux 6.8 kΩ
  ST_TEPLOLUX_6_8KOHM = 1,
  // Electrolux 10 kΩ
  ST_ELECTROLUX_10KOHM = 2,
  // Raychem 13 kΩ
  ST_RAYCHEM_13KOHM = 3,
  // Devi 15 kΩ
  ST_DEVI_15KOHM = 4,
  // Eberie 33 kΩ
  ST_EBERIE_33KOHM = 5,
};

struct ets_state_t {
  uint8_t state_;             // 00 - off, 01 - on
  uint8_t unk01;              // 00 - always 00
  int16_t target_temp_;       // 00fa = 250 = 25.0
  int16_t air_temp_;          // 0114 = 276 = 27.6
  uint16_t unk06;             // 0000 - always 00
  int16_t floor_temp_;        // ffa1 - not connected
  ets_ctl_type_t ctl_type;    // тип управления
  ets_sens_type_t sens_type;  // датчик температуры
  uint16_t unk0C;             // 01c2|0168 ???? (450, 1 194)|(360, 1, 104)
  uint8_t unk0E;              // 00 - always 00
  uint8_t antifreeze;         // 32-on,00-off # антизамерзание
  uint8_t brightness;         // 1e = 30 = 30%
  uint8_t unk10;              // 00 - always 00?
  uint8_t open_wnd_mode;      // 00-off, 01-on
  uint8_t chld_lck;           // 00-on,01-off # блокировка ручного режима
  bool is_off() const { return state_ == 0; }
  float target_temp() const { return __builtin_bswap16(target_temp_) * 0.1f; }
  float air_temp() const { return __builtin_bswap16(air_temp_) * 0.1f; }
  float floor_temp() const { return __builtin_bswap16(floor_temp_) * 0.1f; }
};

struct ets_mode_t {
  enum { UNCHANGED = 0x7F };
  uint8_t state_{UNCHANGED};             // 7f-unchanged,00-turn off,01-turn on
  uint8_t unk01{UNCHANGED};              // 7f - always 7f (unchanged)
  int16_t target_temp_;                  // 010e = 270 = 27.6
  uint16_t unk04{0};                     // 0000 - always 0000
  int16_t air_temp_;                     // 0114 = 276 = 27.6
  uint16_t unk08{0};                     // 0000 - always 0000
  ets_ctl_type_t ctl_type{UNCHANGED};    // тип управления
  ets_sens_type_t sens_type{UNCHANGED};  // датчик температуры
  uint16_t unk0C;                        // always 01c2|0168 (00000001b 11000010b)
  uint8_t unk0E{0};                      // 00 - always 00
  uint8_t antifreeze{UNCHANGED};         // 32-on,00-off
  uint8_t brightness{UNCHANGED};         // 7f, value or 7f for unchanged
  uint8_t unk10{0};                      // 00 - always 00
  uint8_t open_wnd_mode{UNCHANGED};      // 7f-unchanged, 00-off, 01-on
  uint8_t chld_lck{UNCHANGED};           // 7f-unchanged, 00-off,01-on};
  void set_state(bool is_on) { state_ = is_on ? 1 : 0; }
  void target_temp(float value) { target_temp_ = __builtin_bswap16(value * 10); }
  void air_temp(float value) { air_temp_ = __builtin_bswap16(value * 10); }
};
#pragma pack(pop)

}  // namespace ets
}  // namespace esphome
