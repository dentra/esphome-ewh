#pragma once

#include "../rka_api/rka_data.h"

namespace esphome {
namespace ehu {
#pragma pack(push, 1)

enum : uint8_t {
  HUMIDITY_MIN = 30,
  HUMIDITY_MAX = 85,
};

enum ehu_packet_type_t : uint8_t {
  // aa02 0a 01 b7
  // aa02 0a 00 b6
  PACKET_REQ_SET_POWER = rka_api::PACKET_REQ_SET_COMMAND,
  // aa02 0b 01 b8 auto
  // aa02 0b 11 c8
  PACKET_REQ_SET_PRESET = 0x0B,

  // aa02 1a 01 c7 вентилятор минимум
  // aa02 1a 03 c9 вентилятор макс
  PACKET_REQ_SET_SPEED = 0x1A,

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
  PACKET_REQ_SET_MUTE = 0x12,
  // aa021101be включить ионизацию
  // aa021100bd выключить ионизацию
  PACKET_REQ_SET_IONIZER = 0x11,

  // Установка часов:
  // aa031b0000c8 - 00 часов 00 минут
  // aa031b0c00d4 - 12 часов 00 минут
  // aa031b0c0fe3 - 12 часов 15 минут
  PACKET_REQ_SET_CLOCK = 0x1B,

  // aa02 19 55 1a установить влажность CO - 85%
  // aa02 19 50 15 установить влажность 80
  // aa02 19 4b 10 установить влажность 75
  // aa02 19 46 0b установить влажность 70
  // aa02 19 41 06 установить влажность 65
  PACKET_REQ_SET_HUMIDITY = 0x19,

  // aa02176326 яркость подсветки максимальная
  // aa02174205 яркость подсветки средняя
  // aa021721e4 яркость подсветки минимальная
  PACKET_REQ_SET_LED_BRIGHTNESS = 0x17,

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

  // aa03081004c9 - будильник 12 часов 18 минут интервал 5 минут
};

// Response for PACKET_REQ_STATE.
// поля начинающиеся с подчеркивания пока не используются
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
    PRESET_PRANA = 0x0C,       // 0C - Prana Auto
    PRESET_MANUAL = 0x0F,      // 0F - ручной
  } preset;                    // 04
  uint8_t _unknown0510[6];     // 05-10 00 00 00 00 00 00
  uint8_t _fitness;            // 11 Активно при режиме фитнес (bool???)
  uint8_t _fitness_time;       // 12 Время минуты в режиме YOGA или фитнес
  enum : uint8_t {
    WATER_WARM_MIST = 1 << 0,  // 01 - Теплый пар
    WATER_UV = 1 << 1,         // 02 - Ультрафиолетовая лампа
  };
  uint8_t water_flags;  // 13
  bool ionizer : 8;     // 14 Ионизация 00 OFF 01 ON
  bool mute : 8;        // 15 Звук вкл-выкл
  uint8_t _unknown16;   // 16 unknown
  bool lock : 8;        // 17 LOCK физ кнопок On-Off
  uint8_t _display_mode;  // 18 Отображение температуры или времени при выкл состояние
  enum : uint8_t {
    LED_PRESET_OFF = 0,
    LED_PRESET_RANDOM = 1,
    LED_PRESET_BLUE = 2,
    LED_PRESET_GREEN = 3,
    LED_PRESET_WHITE = 4,
  } led_preset;            // 19 Подсветка цвет от 0 до 4
  uint8_t led_brightness;  // 20 Тоже подсветка
  enum : uint8_t {
    LED_MODE_BOTTOM = 1 << 0,  // 01 - нижняя
    LED_MODE_TOP = 1 << 1,     // 02 - верхня
  };
  uint8_t led_mode;           // 21 "Режим демонстрации" это режим подсветки
  uint8_t _led3;              // 22 Подсветка 0A
  uint8_t target_humidity;    // 23 Целевая влажность
  uint8_t fan_speed;          // 24 Скорость вентилятора
  uint8_t clock_hours;        // 25 Часы
  uint8_t clock_minutes;      // 26 Минуты
  uint8_t temperature;        // 27 Датчик Температуры
  uint8_t humidity;           // 28 Датчик влажности %
  bool water_tank_empty : 8;  // 29 01 если нет воды else 00
  uint8_t _unknown3032[3];    // 30-32 00 00 0A
  uint8_t _timer_hours;       // 33 Будильник часы
  uint8_t _timer_minutes;     // 34 Будильник минуты
  uint8_t _unknown3550[16];   // 35-41 00 00 00 00 14 00 00 14 00 00 14 00 00 00 00 00 00
  bool _remote_control : 8;   // 51 Удаленное управление - вкл 1 или выкл 00 (пульт?)
};

// aa3288...
static_assert(sizeof(ehu_state_t) == 0x32 /*len*/ - 1 /*cmd:88*/);

#pragma pack(pop)
}  // namespace ehu
}  // namespace esphome
