# Frame structure

`AA XX YY .. YY ZZ`

- `AA` - Magic prefix. Fixed to `0xAA`
- `XX` - Size of `YY` packet in bytes
- `YY .. YY` - Data packet
- `ZZ` - CRC. Sum of `AA+XX+YY..YY`, clipped to byte.

# Packet Structure

`XX YY .. YY`

- `XX` - Required. Command.
- `YY` - Optional. Command data structure.

# Request commands (from user to device)

Some of request commands have corresponding response with `0x80` mask.

## `01` - Device/manufacturer identification

Zero body.

Returns response command `81` with device/manufacturer identification data.

```
00.00.00.00.00.11 Electrolux EWH
00.00.00.00.00.04 Ballu BWH
00.00.00.00.01.05 Electrolux ETS-16
00.00.00.00.00.07 Electrolux EHU-3810D
```

> This command is executed from cloud server every time it connects to device.

> This command will not executed from cloud server if pairing process already completed and 87 command result is not equals data saved by 06 command call.

## `06` - Save Data (?)

any 16 bytes data structure.

Returns command `86` with 1 byte body, 01 means OK, 00 means ERROR. Later this data can be loaded via `07' command.

> This command used in cloud device pairing process.

## `07` - Load Data (?)

Zero body.

This command is executed from cloud server every time it connects to device. Before pairing process the all data must be filled with zeroes.

Returns response with command `87` with 16 bytes data structure.

```
00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 (initital cloud data)
00.00.00.00.00.00.00.00.00.00.00.00.00.1a.25.04 (cloud data with cloud UID triplet, this sample is 263704)
```

## `08` - Request device state

Unknown 2 bytes data structure with fixed value.

```
10 04
```

> Note: Also will work with zero bytes body.

Returns response with [88 command](#88---device-state).

## `0A` - Change device state

First byte is operation, one of:

- `0` - change mode
- `1` - change clock
- `2` - change timer
- `3` - change bst

> Note: do not return value, so you need to request state via [08 command](#08---request-device-state).

### `00` - Change mode structure

- byte mode - `0` - off, `1` - 700W, `2` - 1300W, `3` - 2000W, `4` - no frost
- byte temperature

### `01` - Change clock

- byte hours
- byte minutes

### `02` - Change timer

- byte hours
- byte minutes
- byte mode - probably mode (`1` - 700W, `2` - 1300W, `3` - 2000W), but from app it is always `1`
- byte tempearture

### `03` - Change bst

- byte state - `0` - off, `1` - on

# Response commands (from device to user)

## `05` - Error

- code - `0` - no error, `1` - invlid CRC, `2` - invalid command

## `09` - Device state (Electrolux)

Received every 30 seconds.

- byte state - `0` - off, `1` - 700W, `2` - 1300W, `3` - 2000W, `4` - timer, `5` - no frost
- byte current_temperature
- byte target_temperature
- byte clock_hours
- byte clock_minutes
- byte timer_hours
- byte timer_minutes
- byte error - `0` - no error, `1` - temperature sensor error, `2-255` - other error
- byte bst - `0` - off, `1` - enabled

## `09` - Device state (Ballu)

Received every 30 seconds.

- byte state - `0` - off, `1` - 1300W, `2` - 2000W
- byte current_temperature
- byte target_temperature
- byte unknown - known value 00
- byte unknown - known value 00
- byte unknown - known value 04
- byte unknown - known value 00
- byte error - `0` - no error, `1` - temperature sensor error, `2-255` - other error

## `88` - Device state

Response to request of [08 command](#08---request-device-state).

Structure is the same as in [09 command].
