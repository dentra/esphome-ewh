# Electrolux Water Heater

[![License][license-shield]][license]
[![ESPHome release][esphome-release-shield]][esphome-release]
[![Open in Visual Studio Code][open-in-vscode-shield]][open-in-vscode]
[![Community Forum][community-forum-shield]][community-forum]
[![Support author][donate-tinkoff-shield]][donate-tinkoff]
[![Support author][donate-boosty-shield]][donate-boosty]
[![PayPal.Me][paypal-me-shield]][paypal-me]

[license-shield]: https://img.shields.io/static/v1?label=License&message=MIT&color=orange&logo=license
[license]: https://opensource.org/licenses/MIT
[esphome-release-shield]: https://img.shields.io/static/v1?label=ESPHome&message=2024.6&color=green&logo=esphome
[esphome-release]: https://GitHub.com/esphome/esphome/releases/
[open-in-vscode-shield]: https://img.shields.io/static/v1?label=+&message=Open+in+VSCode&color=blue&logo=visualstudiocode
[open-in-vscode]: https://open.vscode.dev/dentra/esphome-components
[community-forum-shield]: https://img.shields.io/static/v1.svg?label=%20&message=Forum&style=popout&color=41bdf5&logo=HomeAssistant&logoColor=white
[community-forum]: https://community.home-assistant.io/t/electrolux-water-heater-integration/368498
[donate-tinkoff-shield]: https://img.shields.io/static/v1?label=Support+Author&message=Tinkoff&color=yellow
[donate-tinkoff]: https://www.tinkoff.ru/cf/3dZPaLYDBAI
[donate-boosty-shield]: https://img.shields.io/static/v1?label=Support+Author&message=Boosty&color=red
[donate-boosty]: https://boosty.to/dentra
[paypal-me-shield]: https://img.shields.io/static/v1.svg?label=%20&message=PayPal.Me&logo=paypal
[paypal-me]: https://paypal.me/dentra0

This is a ESPHome component to control Electrolux Water Heater (EWH), Ballu (BWH) and possibly Zanussi (ZWH) boilers using uart protocol.

Control is possible via custom dongle. You can make it yourself or buy a ready made.
For example you can try [Lilygo T-Dongle S3](https://github.com/Xinyuan-LilyGO/T-Dongle-S3), the easest way is to do this is on Aliexpress.
Or look at fully opensource [iot-uni-dongle](https://github.com/dudanov/iot-uni-dongle).

The communucation uart protocol is 100% reversed for EWH and partially for BWH. It is fully described at [reverse.md](reverse.md) file.

At this moment the componet is build using climate platform and allows the following:

- Control current temperature
- Change target boil temperature
- Change boil power to 700W (EWH only)
- Change boil power to 1300W
- Change boil power to 2000W
- Change BST (Bacteria Stop technology) mode (EWH only, for BWH need help for reverse)
- Sync and control internal clock (EWH only)
- Set and start internal timer (EWH only)
- Enter to "No Frost/Atifreeze" mode (EWH only)

> [!CAUTION]
>
> ## ⚠️ WARNING: Everything you do is done entirely at your own peril and risk!

## Build ESPHome firmware

You can download and use example configurations for [EWH](ewh.yaml) and [BWH](bwh.yaml) for ZWH try EWH or BWH instead.

Sample configuration:

```yaml
substitutions:
  # main prefix for all entities
  name: "Water Heater"
  # name of your node
  node_name: "water-heater"
  # use "esp32-s3-devkitc-1" for Lilygo T-Dongle S3, "esp12e" for iot-uni-dongle,
  # "esp8285" for coolrf-heatstick, or your own if you know it
  board: "esp12e"
  # time platform: "sntp" or "homeassistant"
  time_platform: "sntp"
  # SSID of your wifi
  wifi_ssid: !secret wifi_ssid
  # password of your wifi
  wifi_password: !secret wifi_password
  # password for fallback wifi hotspot
  wifi_ap_password: !secret wifi_ap_password
  # version of ewh
  project_version: "master"
  # UART configuration.
  tx_pin: GPIO19 # use TX for iot-uni-dongle or coolrf-heatstick
  rx_pin: GPIO20 # use RX for iot-uni-dongle or coolrf-heatstick

# please do not change packeages order it is very important, just comment/uncomment
packages:
  remote:
    url: https://github.com/dentra/esphome-ewh
    ref: master # cant use substitutions here
    files:
      ## Enable additional WiFi signal sensor. Comment next line to disable.
      - packages/wifi_signal.yaml
      ## Enable additional Uptime sensor. Comment next line to disable.
      - packages/uptime.yaml
      ## Uncomment next line to enable additional restart button.
      - packages/restart.yaml
      ## Uncomment next line to enable additional diagnostic clock sensor
      # - packages/clock.yaml
      ## Uncomment next line to enable additional diagnostic timer sensor
      # - packages/timer.yaml
      ## Uncomment next line to enable standalone web ui
      # - packages/ewh_web.yaml
      ## optional package, uncomment next line to enable experimental cloud support
      # - packages/cloud.yaml
      # Required package, do not comment
      - packages/base.yaml
      # Required package, replace with esp32.yaml/esp8266.yaml if you use ESP32/ESP8266
      - packages/esp32_s3.yaml
```

## Expiremental cloud support

Please see instructions [here](components/rka_cloud/README.md).

## Help needed

When the water is heated to the required temperature, the boiler can enter to an idle mode
(display shows 0 and temperature) but unfortunally I can't get this state yet. Obtaining
this state will allow to make energy calculation. If you know the command that can be sent
to the boiler to get this state or even grab internal boiler firmware binary - it will
be superior. Please let me know it.

## Your thanks

If this project was useful to you, you can buy me a Cup of coffee via
[Card2Card](https://www.tinkoff.ru/cf/3dZPaLYDBAI) or [PayPal](https://paypal.me/dentra0) :)
