# Electrolux Water Heater

[![Open in Visual Studio Code][open-in-vscode-shield]][open-in-vscode]
[![Community Forum][community-forum-shield]][community-forum]
[![PayPal.Me][paypal-me-shield]][paypal-me]

[open-in-vscode-shield]: https://open.vscode.dev/badges/open-in-vscode.svg
[open-in-vscode]: https://open.vscode.dev/dentra/esphome-components

[community-forum-shield]: https://img.shields.io/static/v1.svg?label=%20&message=Forum&style=popout&color=41bdf5&logo=HomeAssistant&logoColor=white
[community-forum]: https://community.home-assistant.io/t/electrolux-water-heater-integration/368498

[paypal-me-shield]: https://img.shields.io/static/v1.svg?label=%20&message=PayPal.Me&logo=paypal
[paypal-me]: https://paypal.me/dentra0


This is a ESPHome component to control Electrolux Water Heater and possibly other boilers (Ballu, Zanussi) using uart protocol.

Control is possible via custom dongle. You can make it yourself or buy a ready made.
For example look at [iot-uni-dongle](https://github.com/dudanov/iot-uni-dongle) it fully opensource and also availale for order.

The protocol is described in [reverse.md](reverse.md).

At this moment the componet is build using climate platform and allows the following:
* Control current temperature
* Change target boil temperature
* Change boil power to 700W
* Change boil power to 1300W
* Change boil power to 2000W
* Change BST (Bacteria Stop technology) mode
* Sync and control internal clock
* Set and start timer
* Enter to no frost mode

## Build ESPHome firmware

Sample configuration:

```yaml
# add this repo
external_components:
  - source: github://dentra/esphome-ewh@2021.12.0

logger:
  # Make sure logging is not using the serial port
  baud_rate: 0

uart:
  tx_pin: TX
  rx_pin: RX
  baud_rate: 9600

climate:
  - platform: ewh
    id: wh
    name: "$name"

    # Sensor, Optional, Bacteria Stop Technology switch
    bst:
      name: $name BST

    # Sensor, Optional, Clock sensor
    clock:
      name: $name Clock

    # Sensor, Optional, Timer sensor
    timer:
      name: $name Timer

# Additionally you could sync time on your boiler
time:
  # chose right platform homeasistant or sntp
  - platform: sntp
    on_time:
      # updates every 30 minutes
      seconds: 0
      minutes: /30
      then:
        - lambda: id(wh).sync_clock();

# Populate service "timer" to home assitant
api:
  services:
    - service: timer
      variables:
        hours: int
        minutes: int
        temperature: int
      then:
        lambda: id(wh).timer(hours, minutes, temperature);

```

For full configuration example, please take a look at [ewh.yaml](ewh.yaml) file.

## Expiremental cloud support

> To get it work you need to get mac address of your original dongle first. For example via command: `echo -e "AT+WSMAC\r\n" | nc <IP> 8899`.

Next, just add `cloud_mac` attribute to `climate.ewh` platform.
```yaml
...
climate:
  ...
  - platform: ewh
  ...
    cloud_mac: "12:34:56:78:90:AB"
    # or from your secrets.yaml
    cloud_mac: !secret dongle_mac
  ...
```

## Known issues

Sometimes a command, for example, to change a mode or temperature is not executed and a second change is required

## Help needed

When the water is heated to the required temperature, the boiler can enter to an idle mode (display shows 0 and temperature) but unfortunally I can't get this state yet. Obtaining this state will allow to make energy calculation.


### Your thanks
If this project was useful to you, you can [buy me](https://paypal.me/dentra0) a Cup of coffee :)
