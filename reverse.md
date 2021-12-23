# Frame structure

`AA XX YY .. YY ZZ`

* `AA` - Magic prefix. Fixed to `0xAA`
* `XX` - Size of `YY` packet in bytes
* `ZZ` - CRC. Sum of `AA+XX+YY..YY`, clipped to byte.

# Packet Structure

`XX YY .. YY`

* `XX` - Required. Command.
* `YY` - Optional. Command data structure.

# Request commands (from user to device)

Some of request commands have corresponding response with `0x80` mask.

## `01` - Unknown

Zero body.

Returns response command `81` with fixed data.

```
00.00.00.00.00.11
```

> Requested 3 times after device connected to server

## `07` - Unknown

Zero body.

Returns response with command `87` with unknown data structure.
```
00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
00.00.00.00.00.00.00.00.00.00.00.00.00.1A.07.32 (0xA1=161, 0x07=7, 0x32=50)
00.00.00.00.00.00.00.00.B9.00.00.00.00.00.00.00 (0xB9=185)
```

> With second result it requested 3 times after device connected to server, then executed command 01, then app thinks that device is online
> With first ant third result it requested infinetely
> Command 06 changes third result to second.
> third result i get when turn off device ofline for two about 2-3 hours.

## `06` - Unknown

Unknown 16 bytes data structure.

> Note: equals to `87` command result.

```
00.00.00.00.00.00.00.00.00.00.00.00.00.1A.07.32
```

Returns command 86 with an unknown 1 byte body:
```
01 (ok?)
```

non repeatable, maby from ititial device pairing process.

## `08` - Request device state

Unknown 2 bytes data structure with fixed value.
```
10 04
```

> Note: Also will work with zero bytes body.

Returns response with [88 command](#88---device-state).

## `0A` - Change device state

First byte is operation, one of:
* `0` - change mode
* `1` - change clock
* `2` - change timer
* `3` - change bst

> Note: do not return value, so you need to request state via [08 command](#08---request-device-state).

### `00` - Change mode structure

* byte mode - `0` - off, `1` - 700W, `2` - 1300W, `3` - 2000W, `4` - no frost
* byte temperature

### `01` - Change clock
* byte hours
* byte minutes

### `02` - Change timer
* byte hours
* byte minutes
* byte mode - probably mode (`1` - 700W, `2` - 1300W, `3` - 2000W), but from app it is always `1`
* byte tempearture

### `03` - Change bst
* byte state - `0` - off, `1` - on

# Response commands (from device to user)

## `05` - Error

* code - `1` - invlid CRC, `2` - invalid command

## `09` - Device state

Received every 30 seconds.

* byte state - `0` - off, `1` - 700W, `2` - 1300W, `3` - 2000W, `4` - timer, `5` - no frost
* byte current_temperature
* byte target_temperature
* byte clock_hours
* byte clock_minutes
* byte timer_hours
* byte timer_minutes
* byte unknown
* byte bst - `0` - off, `1` - enabled

## `88` - Device state

Response to request of [08 command](#08---request-device-state).

Structure is the same as in [09 command](#09--device-state).



