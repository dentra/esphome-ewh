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

  // aa02 16 01 c3
  PACKET_REQ_16 = 0x16,
  // aa02 19 32 f7
  PACKET_REQ_19 = 0x19,

  // aa020f02bd включить UV
  // aa020f00bb выключить UV
  // aa020f01bc warm mist включить
  // aa020f00bb warm mist выключить
  PACKET_REQ_SET_WARM_MIST_UV = 0x0F,
  // aa021401c1 блокировка кнопок на увлажнителе
  // aa021400c0 разблокировка кнопок
  PACKET_REQ_SET_LOCK = 0x14,
  // aa021201bf выключить звук
  // aa021200be включить звук
  PACKET_REQ_SET_SOUND_OFF = 0x12,
  // aa021101be включить ионизацию
  // aa021100bd выключить ионизацию
  PACKET_REQ_SET_IONIZER = 0x11,

  // aa02176326 яркость подсветки максимальная
  // aa02174205 яркость подсветки средняя
  // aa021721e4 яркость подсветки минимальная
  PACKET_REQ_SET_BRIGHTNESS = 0x17,

  // aa021601c3 режим подсветки случайные цвета
  // aa021602c4 режим подсветки синий
  // aa021603c5 режим подсветки зеленый
  // aa021604c6 режим подсветки белый
  PACKET_REQ_SET_LED_PRESET = 0x16,

  // aa022301d0 выбор подсветки - верхняя
  // aa022302d1 выбор подсветки - нижняя
  // aa022303d2 выбор подсветки - верхняя + нижняя
  PACKET_REQ_SET_LED_MODE = 0x23,

  // aa03181e00e3 длительность горения цветов 30
  // aa03181f00e4 длительность гореция цветов 31

  // ---промежуточные значения---
  // aa03185a001f длительность гореция цветов 90

  // aa03180014d9 время переключения цветов 20
  // aa03180015da время переключения цветов 21
  // ---промежуточные значения---
  // aa031800b479 время переключения цветов 180

  // Установка часов:
  // aa031b0000c8 - 00 часов 00 минут
  // aa031b0c00d4 - 12 часов 00 минут
  // aa031b0c0fe3 - 12 часов 15 минут
  PACKET_REQ_SET_CLOCK = 0x1B,
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
    PRESET_MEDITATION = 0x0C,  // 0C - Prana Auto
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
  uint8_t clock_hours;        // 25 Часы
  uint8_t clock_minutes;      // 26 Минуты
  uint8_t temperature;        // 27 Датчик Температуры
  uint8_t humidity;           // 28 Датчик влажности %
  bool water_tank_empty : 8;  // 29 01 если нет воды else 00
  uint8_t unknown3032[3];     // 30-32 00 00 0A
  uint8_t timer_hours;        // 33 Будильник часы
  uint8_t timer_minutes;      // 34 Будильник минуты
  uint8_t unknown3550[16];    // 35-41 00 00 00 00 14 00 00 14 00 00 14 00 00 00 00 00 00
  bool remote_control : 8;    // 51 Удаленное управление - вкл 1 или выкл 00
};

// aa3288...
static_assert(sizeof(ehu_state_t) == 0x32 /*len*/ - 1 /*cmd:88*/);

#pragma pack(pop)
}  // namespace ehu
}  // namespace esphome
