# Expiremental cloud support

To get cloud to work you need to pair your dongle with app or get MAC address and device UID of your original dongle.

## Get mac address and cloud key of your original dongle
```bash
echo -e "AT+WSMAC\r\n" | nc <IP> 8899
```
Where <IP> the IP address of your original dongle. The cloud UID you could grab from app.

## Pairing new device.

Follow instructions from your cloud application to pair new device.

To enable pairing fuctionality, for example you could create virtual switch to enable pairing process:
```yaml
switch:
  - platform: template
    name: "$name Pair"
    turn_on_action:
      lambda: id(cloud).pair(true);
    turn_off_action:
      lambda: id(cloud).pair(false);
```

Or you could bind it to your device hardware button.

## Registering new MAC address and get cloud UID from cloud key using emulators
Open run `emu/rk.py <MAC>` and `emu/mcl.py <MAC>` in different consoles. Where `<MAC>` is your new MAC address you want to register. Then start Home Comfort application and press "+" button to add new device. Wait a minute and look at console with `emu/rk.py <MAC>`, you shouls see someting like:
```
r <<< 06: 00.00.00.00.00.00.00.00.00.00.00.00.00.1a.25.04 (16)
new UID 00.00.00.00.00.00.00.00.00.00.00.00.00.1a.25.04
w >>> 86: 01 (1)
```
So string after the `new UID` is your cloud key. The last 3 bytes of it converted to decimals and joined together is your cloud UID. You can check it with app.

## Configuring

```yaml
ewh_cloud:
    # Optional, MAC address used to communicate with cloud
    # for exmple "E6:DF:66:B0:B9:90", use your own or not declare
    mac: !secret cloud_mac
    # Optional, cloud UID used to communicate with cloud
    # for example "263704", use your own or not declare
    key: !secret cloud_uid
    # Optional, cloud host, default to dongle.rusklimat.ru
    host: !secret cloud_host
    # Optional, cloud port, default to 10001
    port: !secret cloud_port

switch:
  # Control switch to enable/disable cloud connect
  - platform: template
    name: "$name Cloud"
    icon: "mdi:cloud-outline"
    lambda: return id(cloud).is_connected();
    turn_on_action:
      lambda: id(cloud).reconnect();
    turn_off_action:
      lambda: id(cloud).disconnect();
    restore_mode: RESTORE_DEFAULT_OFF
    entity_category: config
```

