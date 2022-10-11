# Electrolux Water Heater

[![Version][version-shield]][version]
[![License][license-shield]][license]
[![ESPHome release][esphome-release-shield]][esphome-release]
[![Open in Visual Studio Code][open-in-vscode-shield]][open-in-vscode]
[![Telegram][telegram-shield]][telegram]
[![Community Forum][community-forum-shield]][community-forum]
[![Support author][donate-me-shield]][donate-me]
[![PayPal.Me][paypal-me-shield]][paypal-me]

[version-shield]: https://img.shields.io/static/v1?label=Version&message=2022.1.2&color=green
[version]: https://github.com/dentra/esphome-tion/releases/

[license-shield]: https://img.shields.io/static/v1?label=License&message=MIT&color=orange&logo=license
[license]: https://opensource.org/licenses/MIT

[esphome-release-shield]: https://img.shields.io/static/v1?label=ESPHome&message=2022.9&color=green&logo=esphome
[esphome-release]: https://github.com/esphome/esphome/releases/

[open-in-vscode-shield]: https://img.shields.io/static/v1?label=+&message=Open+in+VSCode&color=blue&logo=visualstudiocode
[open-in-vscode]: https://open.vscode.dev/dentra/esphome-tion

[telegram-shield]: https://img.shields.io/static/v1?label=+&message=Telegram&logo=telegram
[telegram]: https://t.me/joinchat/psTUBFY5E4swZTAy

[community-forum-shield]: https://img.shields.io/static/v1.svg?label=%20&message=Forum&style=popout&color=41bdf5&logo=HomeAssistant&logoColor=white
[community-forum]: https://community.home-assistant.io/t/electrolux-water-heater-integration/368498

[donate-me-shield]: https://img.shields.io/static/v1?label=+&message=Donate
[donate-me]: https://www.tinkoff.ru/cf/3dZPaLYDBAI

[paypal-me-shield]: https://img.shields.io/static/v1?label=+&message=PayPal.Me&logo=paypal
[paypal-me]: https://paypal.me/dentra0



This is a ESPHome component to control Electrolux Water Heater and possibly other boilers (Ballu, Zanussi) using uart protocol.

Control is possible via custom dongle. You can make it yourself or buy a ready made.
For example look at [iot-uni-dongle](https://github.com/dudanov/iot-uni-dongle) it fully opensource and also availale for order.

The communucation uart protocol is reversed on 99.9% and fully described at [reverse.md](reverse.md) file.

At this moment the componet is build using climate platform and allows the following:
* Control current temperature
* Change target boil temperature
* Change boil power to 700W
* Change boil power to 1300W
* Change boil power to 2000W
* Change BST (Bacteria Stop technology) mode
* Sync and control internal clock
* Set and start internal timer
* Enter to "No Frost/Atifreeze" mode

## Build ESPHome firmware

Sample configuration (available for [download](ewh.yaml)):

```yaml
substitutions:
  # main prefix for all entities
  name: "Water Heater"
  # name of your node
  node_name: "water-heater"
  # use "esp12e" for iot-uni-dongle, "esp8285" for coolrf-heatstick, or your own if you know it
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
  ewh_version: "2022.1.0"

# please do not change packeages order it is very important, just comment/uncomment
packages:
  # required package, do not comment
  ewh: github://dentra/esphome-ewh/ewh-pkg-ewh.yaml@$ewh_version

  ## optional package, uncomment next line to enable additional diagnostic clock sensor
  # clock: github://dentra/esphome-ewh/ewh-pkg-clock.yaml@$ewh_version

  ## optional package, uncomment next line to enable additional diagnostic timer sensor
  # timer: github://dentra/esphome-ewh/ewh-pkg-timer.yaml@$ewh_version

  ## optional package, uncomment next line to enable standalone web ui
  # web: github://dentra/esphome-ewh/ewh-pkg-web.yaml@$ewh_version

  ## optional package, uncomment next line to enable experimental cloud support
  # cloud: github://dentra/esphome-ewh/ewh-pkg-cloud.yaml@$ewh_version

  # required package, do not comment
  core: github://dentra/esphome-ewh/ewh-pkg-core.yaml@$ewh_version
```

## Expiremental cloud support

Please see instructions [here](components/ewh_cloud/README.md).

## Known issues

~~Sometimes a command, for example, to change a mode or temperature is not executed and a second change is required.~~

## Help needed

When the water is heated to the required temperature, the boiler can enter to an idle mode (display shows 0 and temperature) but unfortunally I can't get this state yet. Obtaining this state will allow to make energy calculation. If you know the command that can be sent to the boiler to get this state - it will be superior. Please let me know it.

## Your thanks
If this project was useful to you, you can [buy me](https://paypal.me/dentra0) a Cup of coffee :)
