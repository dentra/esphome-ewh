#!/usr/bin/env python3
import serial
import time
import sys
import yaml

# https://ptelectronics.ru/wp-content/uploads/HF-LPT120-User-Manual-V1.1.120160104.pdf
# https://vestnikmag.ru/osobennosti-primeneniya-wi-fi-modulej-hf-lpb100-hf-lpt100-hf-lpt120/


MAC = "00:00:00:00:00:00"
UID_NULL = "00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00"
UID = UID_NULL
URL = "socket://dongle.rusklimat.ru:10001?logging=debug"

if len(sys.argv) > 1:
    MAC = sys.argv[1]
else:
    with open("secrets.yaml", "r") as stream:
        try:
            yml = yaml.safe_load(stream)
            mac = yml["device_mac2"]
            if mac:
                MAC = mac
            uid = yml["device_uid2"]
            if uid:
                UID = uid
        except yaml.YAMLError as exc:
            pass
        except KeyError as exc:
            pass

print(f"Using MAC address: {MAC}")

SHOW_AT = False
SHOW_AA = 1

CMD_01_REQ_DEV_TYPE = 0x01
CMD_06_REQ_SAVE_DATA = 0x06
CMD_05_RSP_ERROR = 0x05
CMD_07_REQ_LOAD_DATA = 0x07
CMD_08_REQ_STATE = 0x08
CMD_88_RSP_STATE = 0x88
CMD_0A_REQ_SET = 0x0A

OPER_00_STATE = 0x00
OPER_01_CLOCK = 0x01
OPER_02_TIMER = 0x02
OPER_03_BST = 0x03

MODE_00_OFF = 0
MODE_01_700W = 1
MODE_02_1300W = 2
MODE_03_2000W = 3
MODE_04_NO_FROST = 4

STATE_00_OFF = 0
STATE_01_700W = 1
STATE_02_1300W = 2
STATE_03_2000W = 3
STATE_04_TIMER = 4
STATE_05_NO_FROST = 5

FS_ST = "st"  # state
FS_CT = "ct"  # current_temperature
FS_TT = "tt"  # target_temperature
FS_CH = "ch"  # clock_hours
FS_CM = "cm"  # clock_minutes
FS_TH = "th"  # timer_hours
FS_TM = "tm"  # timer_minutes
FS_UNK = "unk"  # unknown
FS_BST = "bst"  # bst

DEV_STATE = {
    FS_ST: 1,
    FS_CT: 38,
    FS_TT: 62,
    FS_CH: 19,
    FS_CM: 10,
    FS_TH: 0,
    FS_TM: 0,
    FS_UNK: 0,
    FS_BST: 0,
}

if MAC == "00:00:00:00:00:00":
    URL = "loop://?logging=info"
    print("Startnig test mode")
DEBUG = URL.startswith("loop://")
ser = serial.serial_for_url(URL)


def write_at(buf: str):
    """Write AT commant to UART"""
    msg = buf.encode("utf-8") + b"\r\n"
    if SHOW_AT:
        print(f"w >>> {msg}")
    if not DEBUG:
        time.sleep(1)
    ser.write(msg)


def write_aa(buf: str):
    """Write AA commant to UART"""
    buf = bytes.fromhex(buf.replace(" ", "").replace(".", ""))

    if SHOW_AA > 0:
        bbb = buf[2:-1]
        cmd = bbb[0]
        if cmd != 0x88 and cmd != 0x87 and cmd != 0x81:
            bbb = bbb[1:]
            print(f"w >>> {cmd:02X}: {bbb.hex('.')} ({len(bbb)})")

    if SHOW_AA > 1:
        print(f"w >>> {buf.hex('.')} ({len(buf)})")

    if not DEBUG:
        time.sleep(1)
    ser.write(buf)


def calc_crc(data: bytes) -> int:
    crc = 0
    for i in range(len(data)):
        crc = crc + data[i]
    return crc & 0xFF


def write_cmd(cmd: int, data: str):
    data = data.replace(" ", "").replace(".", "")
    size = int((len(data) / 2) + 1)
    data = f"AA{size:02x}{cmd:02x}{data}"
    crc = calc_crc(bytes.fromhex(data))
    write_aa(f"{data}{crc:02x}")


def process_aa(buf: bytes):
    """Process AA commant"""
    global DEV_STATE
    global UID

    if SHOW_AA > 1:
        print(f"r <<< {buf.hex('.')} ({len(buf)})")
    # remove aa, sz and crc
    crc = buf[len(buf) - 1]
    buf = buf[2:-1]
    cmd = buf[0]
    buf = buf[1:]
    if SHOW_AA > 0 and cmd != 0x08 and cmd != 0x01 and cmd != 0x07:
        print(f"r <<< {cmd:02X}: {buf.hex('.')} ({len(buf)})")

    if False:  # just pretty print
        pass
    elif cmd == CMD_01_REQ_DEV_TYPE:
        write_cmd(0x81, "00.00.00.00.00.11")
    elif cmd == CMD_06_REQ_SAVE_DATA:
        UID = buf.hex(".")
        print(f"new UID {UID}")
        write_cmd(0x86, "01")
    elif cmd == CMD_07_REQ_LOAD_DATA:
        write_cmd(0x87, UID)
    elif cmd == CMD_08_REQ_STATE:
        # write_aa("AA 0A 88 00 1B 23 14 13 01 02 00 00 A4")
        write_cmd(
            CMD_88_RSP_STATE,
            f"{DEV_STATE[FS_ST]:02X}"
            + f"{DEV_STATE[FS_CT]:02X}{DEV_STATE[FS_TT]:02X}"
            + f"{DEV_STATE[FS_CH]:02X}{DEV_STATE[FS_CM]:02X}"
            + f"{DEV_STATE[FS_TH]:02X}{DEV_STATE[FS_TM]:02X}"
            + f"{DEV_STATE[FS_UNK]:02X}{DEV_STATE[FS_BST]:02X}",
        )
        # print(DEV_STATE)
    elif cmd == CMD_0A_REQ_SET:
        # no reponse to set command
        sub = buf[0]
        buf = buf[1:]
        if sub == OPER_00_STATE:
            DEV_STATE[FS_ST] = buf[0]
            DEV_STATE[FS_TT] = buf[1]
            mode = DEV_STATE[FS_ST]
            if mode == STATE_00_OFF:
                mode = "off"
            elif mode == MODE_01_700W:
                mode = "700W"
            elif mode == MODE_02_1300W:
                mode = "1300W"
            elif mode == MODE_03_2000W:
                mode = "2000W"
            elif mode == MODE_04_NO_FROST:
                mode = "no frost"
                DEV_STATE[FS_ST] = STATE_05_NO_FROST
            else:
                mode = f"unknown {mode:02X}"
            print(
                f"set mode {mode} ({DEV_STATE[FS_ST]:X}), temperature {DEV_STATE[FS_TT]} °C"
            )
        elif sub == OPER_01_CLOCK:
            print(f"set clock to {buf[0]:02d}:{buf[1]:02d}")
            DEV_STATE[FS_CH] = buf[0]
            DEV_STATE[FS_CM] = buf[1]
        elif sub == OPER_02_TIMER:
            DEV_STATE[FS_TH] = buf[0]
            DEV_STATE[FS_TM] = buf[1]
            mode = buf[2]
            taget_temp = buf[3]
            DEV_STATE[FS_ST] = STATE_04_TIMER
            print(
                f'set timer to {DEV_STATE[FS_TH]:02d}:{DEV_STATE[FS_TM]:02d}, mode {mode} and {taget_temp} °C ({buf.hex(".")})'
            )
        elif sub == OPER_03_BST:
            print(f"set BST to {buf[0]}")
            DEV_STATE[FS_BST] = buf[0]
        else:
            print(f'Unknown set command operation {sub:02X}: {buf.hex(".")}')
    else:
        print(
            f'Unknown AA command: {cmd:02X}, data {buf.hex(".")} ({len(buf)}), CRC: {crc:02X}'
        )


def process_at(buf: str):
    """Process AT commant"""
    if SHOW_AT:
        print(f"r <<< {buf}")
    if buf == "NDBGL=0,0":
        write_at("+ok")
    elif buf == "APPVER":
        write_at("=BAIGE-1.8-20170810")
    elif buf == "WSMAC":
        write_at(f"+ok={MAC.replace(':','')}\r\n")
    else:
        print(f"Unknown AT command: {buf}")


def process_buf(buf):
    """Process input buffer"""
    while len(buf) > 3:
        # print("buf: " + buf.hex("."))

        if buf[0] == 0xAA:
            end = buf[1] + 3
            if len(buf) < end:
                # not enought data len
                break
            process_aa(buf[0:end])
            buf = buf[end:]
            continue

        if buf[0] == ord("A") and buf[1] == ord("T") and buf[2] == ord("+"):
            index_r = buf.find(b"\r\n", 3)
            if index_r < 0:
                # not enought data len
                break
            process_at(buf[3:index_r].decode("utf-8"))
            buf = buf[index_r + 2 :]
            continue

        # not enought data len
        break

    return buf


def process():
    """Main processing cycle"""
    buf = bytes()
    while True:
        available = ser.in_waiting
        if available > 0:
            try:
                buf = process_buf(buf + ser.read(available))
            except serial.SerialException as err:
                print(err)
                break


def test_data():
    """Generate test data for loop interface"""
    ser.write(b"AT+NDBGL=0,0\r\n")
    ser.write(b"AT+NDBGL=0,0\r\n")
    ser.write(b"AT+APPVER\r\n")
    ser.write(b"AT+WSMAC\r\n")
    ser.write(b"\xAA\x01\x07\xB2")
    ser.write(b"AT+APPVER\r\n")
    ser.write(b"AT+WSMAC\r\n")
    ser.write(b"\xAA\x03\x08\x10\x04\xC9")
    ser.write(b"AT+WSMAC\r\n")

    write_cmd(CMD_0A_REQ_SET, "00 00 23")

    write_cmd(CMD_0A_REQ_SET, "00 01 23")
    write_cmd(CMD_0A_REQ_SET, "00 02 23")
    write_cmd(CMD_0A_REQ_SET, "00 03 23")
    write_cmd(CMD_0A_REQ_SET, "00 04 23")
    write_cmd(CMD_0A_REQ_SET, "00 05 23")
    write_cmd(CMD_0A_REQ_SET, "01 1119")
    write_cmd(CMD_0A_REQ_SET, "03 01")
    write_cmd(CMD_08_REQ_STATE, "")


if DEBUG:
    SHOW_AT = True
    SHOW_AA = 2
    test_data()

process()
ser.close()
