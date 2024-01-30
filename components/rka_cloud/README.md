# Expiremental cloud support

To get cloud to work you need to pair your dongle with app or get MAC address and device UID of your original dongle.

## Pairing new device.

Follow instructions from your cloud application to pair new device.

To enable pairing fuctionality, you need to enable the pair switch.

## Configuring

```yaml
rka_cloud:
  name: "Cloud"
  # Optional, switch to enable pair function with rusklimat app
  pair:
    name: "Cloud Pair"

  # Options below are are optional and for advanced users only!

  # Optional, MAC address used to communicate with cloud
  # for exmple "11:22:33:44:55:66", default to your ESP MAC address
  mac: !secret cloud_mac
  # Optional, cloud UID used to communicate with cloud
  # for example "123456", default to automatic get from rusklimat
  uid: !secret cloud_uid
  # Optional, cloud host, default to dongle.rusklimat.ru
  host: !secret cloud_host
  # Optional, cloud port, default to 10001
  port: !secret cloud_port
```

| Also you could bind pair switch to your device hardware button.

## Get mac address and cloud key of your original dongle
```bash
echo -e "AT+WSMAC\r\n" | nc <IP> 8899
```
Where <IP> the IP address of your original dongle. The cloud UID you could grab from app.
