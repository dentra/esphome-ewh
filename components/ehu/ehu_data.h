#pragma once

#include "../rka_api/rka_data.h"

namespace esphome {
namespace ehu {
#pragma pack(push, 1)

enum ehu_packet_type_t : uint8_t {
  // aa02 0a 01 b7
  // aa02 0a 00 b6
  PACKET_REQ_SET_POWER = rka_api::PACKET_REQ_SET_COMMAND,
  // aa02 0b 01 b8 auto
  // aa02 0b 11 c8
  PACKET_REQ_SET_PRESET = 0x0B,

  PACKET_REQ_SET_SPEED = 0x1A,
  // aa02 12 00 be
  PACKET_REQ_12 = 0x12,
  // aa02 16 01 c3
  PACKET_REQ_16 = 0x16,
  // aa02 19 32 f7
  PACKET_REQ_19 = 0x19,

  PACKET_REQ_SET_WARM_MIST_UV = 0,
  PACKET_REQ_SET_LOCK = 0,
  PACKET_REQ_SET_SOUND = 0,
  PACKET_REQ_SET_IONIZER = 0,
};

// Response for PACKET_REQ_STATE.
struct ehu_state_t {
  bool power : 8;  // 03
  enum : uint8_t {
    PRESET_AUTO = 0x01,        // 01 - AUTO
    PRESET_HEALTH = 0x02,      // 02 - Здоровье
    PRESET_NIGHT = 0x03,       // 03 - NIGHT
    PRESET_BABY = 0x04,        // 04 - Детский
    PRESET_FITNESS = 0x06,     // 06 - фитнес
    PRESET_YOGA = 0x07,        // 07 - YOGA
    PRESET_MEDITATION = 0x08,  // 08 - Медитация
    PRESET_MANUAL = 0x0F,      // 0F - ручной
  } preset_mode;               // 04
  uint8_t unknown0510[6];      // 05-10 00 00 00 00 00 00
  uint8_t fitness;             // 11 Активно при режиме фитнес (bool???)
  uint8_t fitness_time;        // 12 Время минуты в режиме YOGA или фитнес
  enum : uint8_t {
    WATER_WARM_MIST = 1 << 0,  // 01 - Теплый пар
    WATER_UV = 1 << 1,         // 02 - Ультрафиолетовая лампа
  };
  uint8_t water_flags;  // 13
  bool ionizer : 8;     // 14 Ионизация 00 OFF 01 ON
  bool sound : 8;       // 15 Звук вкл-выкл
  uint8_t unknown16;    // 16 unknown
  bool lock : 8;        // 17 LOCK физ кнопок On-Off
  uint8_t display_mode;  // 18 Отображение температуры или времени при выкл состояние
  uint8_t led1;               // 19 Подсветка цвет от 0 до 4 (бак или нижняя ???)
  uint8_t led2;               // 20 Тоже подсветка (бак или нижняя ???)
  uint8_t demo_mode;          // 21 Режим демонстрации (bool???)
  uint8_t led3;               // 22 Подсветка 0A
  uint8_t manual_value;       // 23 Значение ручного режима
  uint8_t fan_speed;          // 24 Скорость вентилятора
  uint8_t hours;              // 25 Часы
  uint8_t minutes;            // 26 Минуты
  uint8_t temperature;        // 27 Датчик Температуры
  uint8_t humidity;           // 28 Датчик влажности %
  bool water_tank_empty : 8;  // 29 01 если нет воды else 00
  uint8_t unknown3032[3];     // 30-32 00 00 0A
  uint8_t alarm_hours;        // 33 Будильник часы
  uint8_t alarm_minutes;      // 34 Будильник минуты
  uint8_t unknown3550[16];    // 35-41 00 00 00 00 14 00 00 14 00 00 14 00 00 00 00 00 00
  bool remote_control : 8;    // 51 Удаленное управление - вкл 1 или выкл 00
};

// aa3288...
static_assert(sizeof(ehu_state_t) == 0x32 /*len*/ - 1 /*cmd:88*/);

#pragma pack(pop)
}  // namespace ehu
}  // namespace esphome
