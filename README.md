# Electrolux Water Heater

[![Open in Visual Studio Code](https://open.vscode.dev/badges/open-in-vscode.svg)](https://open.vscode.dev/dentra/esphome-components)

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
  - source: github://dentra/esphome-ewh

logger:
  # Make sure logging is not using the serial port
  baud_rate: 0

uart:
  tx_pin: TX
  rx_pin: RX
  baud_rate: 9600

climate:
  - platform: ewh
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
```

For full configuration example, please take a look at [ewh.yaml](ewh.yaml) file.

## Help needed

When the water is heated to the required temperature, the boiler can enter to an idle mode (display shows 0 and temperature) but unfortunally I can't get this state yet. Obtaining this state will allow to make energy calculation.


### Your thanks
If this project was useful to you, you can [buy me](https://paypal.me/dentra0) a Cup of coffee :)
