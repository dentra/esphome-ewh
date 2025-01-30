# Electrolux Water Heater and more...

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

This is a set of ESPHome components to control Rusklimat devices using UART protocol:

- Electrolux Water Heater (EWH)
- Ballu Water Heater (BWH)
- Zanussi Water Heater (ZWH)
- Electrolux Humidifier (EHU) (early beta!)
- Electrolux Thermostat (ETS) (experimental!)

Control is possible via custom dongle. You can make it yourself or buy a ready made.
For example you can try [Lilygo T-Dongle S3](https://github.com/Xinyuan-LilyGO/T-Dongle-S3), the easest way is to do this is on Aliexpress.

The communication UART protocol is 100% reversed for EWH and partially for BWH. It is fully described at [reverse.md](reverse.md) file.

Water heater components are build using climate platform and allows the following:

- Control current temperature
- Change target boil temperature
- Change boil power to 700W (EWH only, for BWH need help for reverse)
- Change boil power to 1300W
- Change boil power to 2000W
- Change BST (Bacteria Stop technology) mode (EWH only, for BWH need help for reverse)
- Sync and control internal clock (EWH only)
- Set and start internal timer (EWH only)
- Enter to "No Frost/Atifreeze" mode (EWH only)

Humidifier components are build using climate platform and allows the following:

- Control on/off
- Control fan speed
- Control internal presets
- Monitor warm mist
- Monitor UV
- Monitor ionizer
- Monitor lock
- Monitor sound
- Monitor humidity
- Monitor temperature
- Monitor empty water tank

> [!CAUTION]
>
> ## ⚠️ WARNING: Everything you do is done entirely at your own peril and risk!

## Build ESPHome firmware

You can download and use example configurations for:

- [EWH](ewh.yaml)
- [BWH](bwh.yaml)
- ZWH try EWH or BWH
- [EHU](ehu.yaml)
- [ETS](ets.yaml)

## Expiremental cloud support

Please see instructions [here](components/rka_cloud/README.md).

## Help needed

When the water is heated to the required temperature, the boiler can enter to an idle mode
(display shows 0 and temperature) but unfortunately I can't get this state yet. Obtaining
this state will allow to make energy calculation. If you know the command that can be sent
to the boiler to get this state or even grab internal boiler firmware binary - it will
be superior. Please let me know it.

## Your thanks

If this project was useful to you, you can buy me a Cup of coffee via
[Card2Card](https://www.tinkoff.ru/cf/3dZPaLYDBAI) or [PayPal](https://paypal.me/dentra0) :)
