#!/usr/bin/env python3
# pylint: disable=missing-module-docstring
# pylint: disable=missing-function-docstring
# pylint: disable=missing-class-docstring
import time
import sys
import logging
import yaml
import serial

# https://ptelectronics.ru/wp-content/uploads/HF-LPT120-User-Manual-V1.1.120160104.pdf
# https://vestnikmag.ru/osobennosti-primeneniya-wi-fi-modulej-hf-lpb100-hf-lpt100-hf-lpt120/

_LOGGER = logging.getLogger()


class Dongle:
    def write_cmd(self, cmd: int, data: str):
        pass


class Device:
    CMD_01_REQ_DEV_TYPE = 0x01
    CMD_81_RSP_DEV_TYPE = 0x81
    CMD_06_REQ_SAVE_DATA = 0x06
    CMD_86_RSP_SAVE_DATA = 0x86
    CMD_05_RSP_ERROR = 0x05
    CMD_07_REQ_LOAD_DATA = 0x07
    CMD_87_RSP_LOAD_DATA = 0x87
    CMD_08_REQ_STATE = 0x08
    CMD_88_RSP_STATE = 0x88
    CMD_09_RSP_STATE = 0x09
    CMD_0A_REQ_SET = 0x0A

    state = 1
    current_temperature = 40
    target_temperature = 60
    error = 1
    """
    0: no error
    1: temperature sensor error
    2-255: other error
    """

    data = "00.00.00.00.00.00.00.00.00.00.00.00.00.00.00.00"

    def __init__(self, dongle: Dongle):
        self._dongle = dongle

    def get_name(self):
        pass

    def _write_cmd(self, cmd: int, data: str):
        self._dongle.write_cmd(cmd, data)

    def write_type(self):
        pass

    def write_state(self, cmd: int):
        pass

    def process_cmd(self, cmd: int, buf: bytes) -> bool:
        if cmd == self.CMD_01_REQ_DEV_TYPE:
            print("request device type")
            self.request_dev_type(buf)
            return True

        if cmd == self.CMD_06_REQ_SAVE_DATA:
            print("request save data")
            self.request_save_data(buf)
            return True

        if cmd == self.CMD_07_REQ_LOAD_DATA:
            print("request load data")
            self.request_load_data(buf)
            return True

        if cmd == self.CMD_08_REQ_STATE:
            print("request state")
            self.write_state(self.CMD_88_RSP_STATE)
            return True

        if cmd == self.CMD_0A_REQ_SET:
            print("request set command")
            self.request_set(buf)
            return True

        return False

    def request_dev_type(self, buf: bytes):  # pylint: disable=unused-argument
        self.write_type()

    def request_save_data(self, buf: bytes):
        self.data = buf.hex(".")
        self._write_cmd(self.CMD_86_RSP_SAVE_DATA, "01")

    def request_load_data(self, buf: bytes):  # pylint: disable=unused-argument
        self._write_cmd(self.CMD_87_RSP_LOAD_DATA, self.data)

    def request_state(self, buf: bytes):  # pylint: disable=unused-argument
        self.write_state(self.CMD_88_RSP_STATE)

    def request_set(self, buf: bytes):
        operation = buf[0]
        buf = buf[1:]
        if not self._exec_operation(operation, buf):
            _LOGGER.warning(
                "Unknown set command operation %02X: %s", operation, buf.hex(".")
            )

    def _exec_operation(self, operation: int, buf: bytes) -> bool:
        pass


class EWH(Device):
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

    STATE_MAP = {}

    clock_hours = 19
    clock_minutes = 10
    timer_hours = 0
    timer_minutes = 0
    bst = 0

    def get_name(self):
        return "Electrolux Water Heater"

    def write_type(self):
        self._write_cmd(self.CMD_81_RSP_DEV_TYPE, "00.00.00.00.00.11")

    def write_state(self, cmd: int = -1):
        if cmd == -1:
            cmd = self.CMD_09_RSP_STATE

        self._write_cmd(
            cmd,
            f"{self.state:02X}"
            + f"{self.current_temperature:02X}{self.target_temperature:02X}"
            + f"{self.clock_hours:02X}{self.clock_minutes:02X}"
            + f"{self.timer_hours:02X}{self.timer_minutes:02X}"
            + f"{self.error:02X}{self.bst:02X}",
        )

    def _exec_operation(self, operation: int, buf: bytes) -> bool:
        if operation == self.OPER_00_STATE:
            self._set_mode(buf)
            return True

        if operation == self.OPER_01_CLOCK:
            self._set_clock(buf)
            return True

        if operation == self.OPER_02_TIMER:
            self._set_timer(buf)
            return True

        if operation == self.OPER_03_BST:
            self._set_bst(buf)
            return True

        return False

    def _set_mode(self, buf: bytes):
        mode = buf[0]
        self.target_temperature = buf[1]

        if mode == self.MODE_00_OFF:
            # reset error on off
            self.error = 0

        if mode == self.MODE_00_OFF:
            self.state = self.STATE_00_OFF
        elif mode == self.MODE_01_700W:
            self.state = self.STATE_01_700W
        elif mode == self.MODE_02_1300W:
            self.state = self.STATE_02_1300W
        elif mode == self.MODE_03_2000W:
            self.state = self.STATE_03_2000W
        elif mode == self.MODE_04_NO_FROST:
            self.state = self.STATE_05_NO_FROST
        else:
            self.state = mode

        _LOGGER.info(
            "set mode %s (%d), temperature %d °C",
            self.STATE_MAP.get(self.state, f"unknown {mode:02X}"),
            self.state,
            self.target_temperature,
        )

    def _set_clock(self, buf: bytes):
        self.clock_hours = buf[0]
        self.clock_minutes = buf[1]
        _LOGGER.info("set clock to %02d:%02d", self.clock_hours, self.clock_minutes)

    def _set_timer(self, buf: bytes):
        self.timer_hours = buf[0]
        self.timer_minutes = buf[1]
        mode = buf[2]
        taget_temp = buf[3]
        self.state = self.STATE_04_TIMER
        _LOGGER.info(
            "set timer to %02d:%02d, mode %d and %d °C (%s)",
            self.timer_hours,
            self.timer_minutes,
            mode,
            taget_temp,
            buf.hex("."),
        )

    def _set_bst(self, buf: bytes):
        self.bst = buf[0]
        _LOGGER.info("set BST to %d", self.bst)


class BWH(Device):
    OPER_00_STATE = 0x00
    MODE_OFF = 0
    MODE_1300W = 1
    MODE_2000W = 2
    STATE_MAP = {}

    def get_name(self):
        return "Ballu Water Heater"

    def write_type(self):
        self._write_cmd(self.CMD_81_RSP_DEV_TYPE, "00.00.00.00.00.04")

    def write_state(self, cmd):
        self._write_cmd(
            cmd,
            f"{self.state:02X}{self.current_temperature:02X}{self.target_temperature:02X}"
            + "0203 04 0100",
        )

    def _exec_operation(self, operation: int, buf: bytes) -> bool:
        if operation == self.OPER_00_STATE:
            self._set_mode(buf)
            return True
        return False

    def _set_mode(self, buf: bytes):
        mode = buf[0]
        self.target_temperature = buf[1]

        if mode == self.MODE_OFF:
            # reset error on off
            self.error = 0

        self.state = mode

        _LOGGER.info(
            "set mode %s (%d), temperature %d °C",
            self.STATE_MAP.get(self.state, f"unknown {mode:02X}"),
            self.state,
            self.target_temperature,
        )


class DongleEmu(Dongle):
    _ser = None
    DEBUG = False
    SHOW_AT = False
    SHOW_AA = 1

    def __init__(self, mac: str = "00:00:00:00:00:00"):
        self._device = EWH(self)
        self._mac = mac

    def __del__(self):
        self.close()

    def close(self):
        if self._ser is not None:
            self._ser.close()

    def run(self, url: str):
        self._ser = serial.serial_for_url(url)
        buf = bytes()
        oldtime = time.time()
        while True:
            available = self._ser.in_waiting
            if available > 0:
                try:
                    buf = self._process_buf(buf + self._ser.read(available))
                except serial.SerialException as err:
                    _LOGGER.error(err)
                    break
            elif time.time() - oldtime > 30:
                oldtime = time.time()
                self._device.write_state()

    def _process_buf(self, buf):
        """Process input buffer"""
        while len(buf) > 3:
            # print("buf: " + buf.hex("."))

            if buf[0] == 0xAA:
                end = buf[1] + 3
                if len(buf) < end:
                    # not enought data len
                    break
                self._process_aa(buf[0:end])
                buf = buf[end:]
                continue

            if buf[0] == ord("A") and buf[1] == ord("T") and buf[2] == ord("+"):
                index_r = buf.find(b"\r\n", 3)
                if index_r < 0:
                    # not enought data len
                    break
                self._process_at(buf[3:index_r].decode("utf-8"))
                buf = buf[index_r + 2 :]
                continue

            print(f"unknown input {buf}")

            # not enought data len
            break

        return buf

    def _process_at(self, buf: str):
        """Process AT commant"""
        if self.SHOW_AT:
            print(f"r <<< {buf}")
        if buf == "NDBGL=0,0":
            self._write_at("+ok")
        elif buf == "APPVER":
            self._write_at("=BAIGE-1.8-20170810")
        elif buf == "WSMAC":
            self._write_at(f"+ok={self._mac.replace(':','')}\r\n")
        else:
            print(f"Unknown AT command: {buf}")

    def _write_at(self, buf: str):
        """Write AT commant to UART"""
        msg = buf.encode("utf-8") + b"\r\n"
        if self.SHOW_AT:
            print(f"w >>> {msg}")
        if not self.DEBUG:
            time.sleep(1)
        self._ser.write(msg)

    def _write_aa(self, buf: str):
        """Write AA commant to UART"""
        buf = bytes.fromhex(buf.replace(" ", "").replace(".", ""))

        if self.SHOW_AA > 0:  # cmd != 0x88 and cmd != 0x87 and cmd != 0x81:
            bbb = buf[2:-1]
            cmd = bbb[0]
            bbb = bbb[1:]
            print(f"w >>> {cmd:02X}: {bbb.hex('.')} ({len(bbb)})")
        if self.SHOW_AA > 1:
            print(f"w >>> {buf.hex('.')} ({len(buf)})")

        # if not DEBUG:
        #     time.sleep(1)
        self._ser.write(buf)

    def _calc_crc(self, data: bytes) -> int:
        """Calculate CRC"""
        crc = 0
        for _, byte in enumerate(data):
            crc = crc + byte
        return crc & 0xFF

    def write_cmd(self, cmd: int, data: str):
        """Write AA command"""
        data = data.replace(" ", "").replace(".", "")
        size = int((len(data) / 2) + 1)
        data = f"AA{size:02x}{cmd:02x}{data}"
        crc = self._calc_crc(bytes.fromhex(data))
        self._write_aa(f"{data}{crc:02x}")

    def _process_aa(self, buf: bytes):
        """Process AA commant"""
        if self.SHOW_AA > 1:
            print(f"r <<< {buf.hex('.')} ({len(buf)})")
        # remove aa, sz and crc
        crc = buf[len(buf) - 1]
        buf = buf[2:-1]
        cmd = buf[0]
        buf = buf[1:]
        if self.SHOW_AA > 0:  # and cmd != 0x08 and cmd != 0x01 and cmd != 0x07:
            print(f"r <<< {cmd:02X}: {buf.hex('.')} ({len(buf)})")

        if not self._device.process_cmd(cmd, buf):
            _LOGGER.warning(
                "Unknown AA command: %02X, data %s (%d), CRC: %02X",
                cmd,
                buf.hex("."),
                len(buf),
                crc,
            )

    def set_mac(self, mac: str):
        self._mac = mac

    def get_mac(self):
        return self._mac

    def set_typ(self, typ: str):
        if typ == "BWH":
            self._device = BWH(self)

    def get_typ(self):
        return self._device.get_name()

    def set_uid(self, uid: str):
        pre = self._device.data[:-8]
        data = f"{pre}{int(uid[0:2]):02X}.{int(uid[2:4]):02X}.{int(uid[4:6]):02X}"
        self._device.data = data

    def get_uid(self):
        uid = self._device.data
        res = f"{int(uid[39:41], 16):02d}{int(uid[42:44], 16):02d}{int(uid[45:47], 16):02d} ({uid})"
        return res


def main(argv: list[str]):
    emu = DongleEmu()

    if len(argv) > 1:
        emu.set_mac(argv[1])
    else:
        try:
            with open("secrets.yaml", mode="r", encoding="utf8") as stream:
                try:
                    yml = yaml.safe_load(stream)
                    mac = str(yml["cloud_mac"])
                    if mac:
                        emu.set_mac(mac)
                    typ = str(yml["cloud_typ"])
                    if typ:
                        emu.set_typ(typ)
                    uid = str(yml["cloud_uid"])
                    if uid:
                        emu.set_uid(uid)
                except (yaml.YAMLError, KeyError):
                    pass
        except FileNotFoundError:
            pass

    print(f"Cloud MAC: {emu.get_mac()}")
    print(f"Cloud UID: {emu.get_uid()}")
    print(f"Device : {emu.get_typ()}")

    url = "socket://dongle.rusklimat.ru:10001?logging=debug"
    if emu.get_mac() == "00:00:00:00:00:00":
        url = "loop://?logging=info"
        print("Startnig test mode")
    emu.DEBUG = url.startswith("loop://")
    if emu.DEBUG:
        emu.SHOW_AT = True
        emu.SHOW_AA = 2
        # test_data()

    emu.run(url)
    # SHOW_AA = 2
    # write_cmd(CMD_87_RSP_LOAD_DATA, UID_NULL)


# def test_data():
#     """Generate test data for loop interface"""
#     ser.write(b"AT+NDBGL=0,0\r\n")
#     ser.write(b"AT+NDBGL=0,0\r\n")
#     ser.write(b"AT+APPVER\r\n")
#     ser.write(b"AT+WSMAC\r\n")
#     ser.write(b"\xAA\x01\x07\xB2")
#     ser.write(b"AT+APPVER\r\n")
#     ser.write(b"AT+WSMAC\r\n")
#     ser.write(b"\xAA\x03\x08\x10\x04\xC9")
#     ser.write(b"AT+WSMAC\r\n")

#     write_cmd(EWH.CMD_0A_REQ_SET, "00 00 23")

#     write_cmd(EWH.CMD_0A_REQ_SET, "00 01 23")
#     write_cmd(EWH.CMD_0A_REQ_SET, "00 02 23")
#     write_cmd(EWH.CMD_0A_REQ_SET, "00 03 23")
#     write_cmd(EWH.CMD_0A_REQ_SET, "00 04 23")
#     write_cmd(EWH.CMD_0A_REQ_SET, "00 05 23")
#     write_cmd(EWH.CMD_0A_REQ_SET, "01 1119")
#     write_cmd(EWH.CMD_0A_REQ_SET, "03 01")
#     write_cmd(EWH.CMD_08_REQ_STATE, "")

if __name__ == "__main__":
    main(sys.argv)
