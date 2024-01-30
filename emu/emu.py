#!/usr/bin/env python3
# pylint: disable=missing-module-docstring
# pylint: disable=missing-function-docstring
# pylint: disable=missing-class-docstring

import abc
import dataclasses
import logging
import os
import sys
import signal
import struct
import time

from typing import Final, Any, Callable

import serial
import yaml

_LOGGER = logging.getLogger()


def uid_data(uid: int):
    uid = list(str(uid))
    if len(uid) != 6:
        return "00 00 00"
    dat = ""
    for i in range(0, len(uid), 2):
        num = int(uid[i]) * 10 + int(uid[i + 1])
        dat += f"{num:02x}"
    return dat


def onoff(state) -> str:
    return "OFF" if state else "ON"


class Dongle:
    @abc.abstractmethod
    def write_cmd(self, cmd: int, data: str):
        pass


class Device:
    _reqs: dict[int, Callable] = {}

    def __init__(self, dongle: Dongle):
        self._dongle = dongle

    @property
    def name(self):
        return None

    def _write_cmd(self, cmd: int, data: bytes):
        self._dongle.write_cmd(cmd, data)

    def process_cmd(self, cmd: int, buf: bytes) -> bool:
        return False

    def process_at(self, buf: str) -> str:
        return False


@dataclasses.dataclass
class CfgDevice:
    uid: int = None
    mac: str = "00:00:00:00:00:00"
    name: str = "unknown"
    byte_order: str = "little-endian"
    unchanged: dict[str, int] = dataclasses.field(default_factory=dict)

    def __post_init__(self):
        self.mac = self.mac.replace(":", "")

    @property
    def struct_fmt(self):
        return ">" if self.byte_order == "big-endian" else "<"


STATE_ITEM_STRUCT_TYPE: Final = {
    "uint8": "B",
    "int8": "b",
    "bool": "?",
    "int16": "h",
    "uint16": "H",
    "int32": "i",
    "uint32": "I",
    "int64": "q",
    "uint64": "Q",
}


class CfgItemBase:
    def _post_init_lambda(
        self, lambdas: list[Callable], _lambda
    ):  # TODO  -> str | Callable:
        if _lambda is not None:
            prefix = "!lambda "
            if isinstance(_lambda, str) and _lambda.startswith(prefix):
                return lambdas[int(_lambda[len(prefix) :])]
        return _lambda


@dataclasses.dataclass
class CfgStateItem(CfgItemBase):
    name: str
    device: CfgDevice = dataclasses.field(repr=False)
    lambdas: list[Callable] = dataclasses.field(repr=False, default_factory=list)
    type: str = "uint8"
    data: Any = dataclasses.field(default=0)  # TODO  int | bytes:
    show: bool = True
    unit: str = ""
    format: str = None
    eval: Any = None  # TODO Callable | str
    map: dict[int, str] = None
    unchanged: int = None

    def __post_init__(self):
        if self.unchanged is None and self.type in self.device.unchanged:
            self.unchanged = self.device.unchanged[self.type]

        self.format = self._post_init_lambda(self.lambdas, self.format)
        self.eval = self._post_init_lambda(self.lambdas, self.eval)
        self.data = self._post_init_lambda(self.lambdas, self.data)
        if callable(self.data):
            self.data = self.data(self.device)

    @property
    def _is_data(self) -> bool:
        return self.type == "data"

    @property
    def value(self):  # TODO  -> int | bytes:
        data = self.data
        if self._is_data and isinstance(data, str):
            data = data.replace(" ", "").replace(".", "")
            data = bytes.fromhex(data)
        return data

    @property
    def struct_fmt(self):
        if self._is_data:
            return f"{len(self.value)}s"
        return STATE_ITEM_STRUCT_TYPE[self.type]

    def __str__(self) -> str:
        data = self.value
        if callable(self.eval):
            data = self.eval(data)
        if isinstance(data, bytes):
            return data.hex(" ")
        if isinstance(self.map, dict):
            return self.map[data]
        if isinstance(self.format, str):
            return f"{{:{self.format}}}".format(data)
        if callable(self.format):
            return self.format(data)
        return str(data)


@dataclasses.dataclass
class CfgCmdAtItem(CfgItemBase):
    device: CfgDevice = dataclasses.field(repr=False)
    lambdas: list[Callable] = dataclasses.field(repr=False, default_factory=list)
    request: str = None
    response: Any = None  # str | Callable

    def __post_init__(self):
        self.response = self._post_init_lambda(self.lambdas, self.response)
        if callable(self.response):
            self.response = self.response(self.device)


class EmuConfig:
    _device = CfgDevice
    _state: dict[str, CfgStateItem] = {}
    _commands = {}
    _commands_at: list[CfgCmdAtItem] = []
    _commands_at_tpl: Final[dict[str, dict[str, Any]]] = {
        "ndbgl": {"request": "NDBGL=0,0", "response": "+ok"},
        "appver": {"request": "APPVER", "response": "=BAIGE-1.8-20170810"},
        "wsmac": {"request": "WSMAC", "response": lambda x: f"+ok={x.mac}\r\n"},
    }
    _tests = []
    _max_request_size = 0
    _lambdas = []

    def _apply(self, cfg: dict[str, Any]):
        self._device = CfgDevice(**cfg["device"])

        self._state.update(cfg["state"])
        self._commands.update(cfg["commands"])

        if "tests" in cfg:
            self._tests = cfg["tests"]

        cmdat = self._commands_at_tpl.copy()
        if "commands_at" in cfg:
            cmdat.update(cfg["commands_at"])

        for k, v in cmdat.items():
            m = {} if v is None else v
            c = CfgCmdAtItem(device=self._device, lambdas=self._lambdas, **m)
            self._commands_at.append(c)

        for k, v in cfg["state"].items():
            m = {} if v is None else v
            c = CfgStateItem(k, device=self._device, lambdas=self._lambdas, **m)
            self._state[k] = c

        for _, val in self._commands.items():
            req = val["request"]
            if "struct" in req:
                fmt = self._struct_unpack_fmt(req["struct"])
                self._max_request_size = max(
                    self._max_request_size, struct.calcsize(fmt)
                )
            if not isinstance(req["code"], list):
                req["code"] = [req["code"]]

        _LOGGER.info(self._device)
        if _LOGGER.isEnabledFor(logging.DEBUG):
            for x in self._commands_at:
                _LOGGER.debug(repr(x))
            for x in self._state.values():
                _LOGGER.debug(repr(x))
            _LOGGER.debug("max_request_size: %d", self._max_request_size)

        return True

    def read_config(self, cfg_file: str):
        secrets = {}
        secrets_file = os.path.dirname(os.path.realpath(cfg_file)) + "/secrets.yaml"
        if os.path.exists(secrets_file):
            with open(secrets_file, "r", encoding="utf-8") as stream:
                try:
                    secrets.update(yaml.safe_load(stream))
                except yaml.YAMLError as exc:
                    _LOGGER.error(exc)

        def secret_ctor(loader: yaml.SafeLoader, node: yaml.nodes.ScalarNode) -> str:
            if node.value in secrets:
                return secrets[node.value]
            _LOGGER.warning("No %s secret found", node.value)
            return ""

        def lambda_ctor(loader: yaml.SafeLoader, node: yaml.nodes.ScalarNode) -> str:
            code = f"def lambda_{len(self._lambdas)}(x) -> str:\n    "
            code += "\n    ".join(str(node.value).split("\n"))
            code += f"\nlambdas.append(lambda_{len(self._lambdas)})"
            globs = {
                "lambdas": self._lambdas,
                "log": _LOGGER,
                "uid_data": uid_data,
                "onoff": onoff,
            }
            exec(code, globs, {})  # pylint: disable=exec-used
            return f"!lambda {len(self._lambdas) - 1}"

        def secret_loader():
            loader = yaml.SafeLoader
            loader.add_constructor("!secret", secret_ctor)
            loader.add_constructor("!lambda", lambda_ctor)
            return loader

        with open(cfg_file, "r", encoding="utf-8") as stream:
            try:
                config = yaml.load(stream, Loader=secret_loader())
                return self._apply(config)
            except yaml.YAMLError as exc:
                _LOGGER.error(exc)
                return False

    def run_tests(self, emu):
        for tst in self._tests:
            emu.test(tst)

    def __str__(self):
        return str(self._device)

    @property
    def device_name(self):
        return self._device.name

    @property
    def max_request_size(self):
        return self._max_request_size

    def find_command(self, cmd: int, data: bytes):
        chk = [cmd] + list(data)
        for key, val in sorted(
            self._commands.items(),
            key=lambda item: item[1]["request"]["code"],
            reverse=True,
        ):
            code = val["request"]["code"]
            if code == chk[: len(code)]:
                return key, val, len(code)
        return None, None, None

    def at_result(self, cmd: str) -> str:
        for val in self._commands_at:
            if val.request == cmd:
                return val.response
        return None

    def struct_pack(self, fields: list[str]) -> bytes:
        fmt = self._device.struct_fmt
        data = []
        for field in fields:
            if isinstance(field, dict):
                field = list(field.keys())[0]
            item = self._state[field]
            fmt += item.struct_fmt
            data.append(item.value)
        return struct.pack(fmt, *data)

    def _struct_unpack_fmt(self, fields: list[str]) -> str:
        fmt = self._device.struct_fmt
        for field in fields:
            if isinstance(field, dict):
                field = list(field.keys())[0]
            item = self._state[field]
            fmt += item.struct_fmt
        return fmt

    def struct_unpack(self, fields: list[str], buf: bytes) -> None:
        fmt = self._struct_unpack_fmt(fields)
        data = struct.unpack(fmt, buf)
        for key, val in enumerate(data):
            field_key = fields[key]
            if isinstance(field_key, dict):
                field_key = list(field_key.keys())[0]
            field = self._state[field_key]
            if field.unchanged is None or field.unchanged != val:
                field.data = val

    def show_state(self):
        max_key_len = 0
        for key in self._state:
            max_key_len = max(max_key_len, len(key))

        for key, val in self._state.items():
            if not val.show:
                continue
            fmt = f"  %-{max_key_len}s: %s %s"
            if key.startswith("unk"):
                dat = val.value
                if dat != 0:
                    fmt += f" ({dat} / 0x{dat:X})"
            _LOGGER.info(fmt, key, str(val), val.unit)


class YamlDevice(Device):
    def __init__(self, dongle: Dongle, cfg: EmuConfig):
        super().__init__(dongle)
        self._cfg = cfg

    @property
    def name(self):
        return self._cfg.device_name

    @property
    def max_request_size(self):
        return self._cfg.max_request_size

    def _proc_req(self, cmd_key: str, cmd: dict, buf: bytes) -> bool:
        req = cmd["request"]
        if "struct" in req:
            self._cfg.struct_unpack(req["struct"], buf)
            self._cfg.show_state()
        return self._proc_rsp(cmd_key, cmd)

    def _proc_rsp(self, cmd_key: str, cmd: dict) -> bool:
        rsp = cmd["response"]
        data = self._cfg.struct_pack(rsp["struct"])
        if data is not None:
            self._write_cmd(rsp["code"], data)
            self._cfg.show_state()
            return True
        cmd_code = cmd["request"]["code"]
        _LOGGER.error("No reposnse found for %s [%02X] command", cmd_key, cmd_code)
        return False

    def process_cmd(self, cmd: int, buf: bytes) -> bool:
        key, cmd, sub = self._cfg.find_command(cmd, buf)
        if cmd is None:
            return False
        if sub > 1:
            buf = buf[sub - 1 :]
        _LOGGER.info("Processing: %s [%s]", key, buf.hex(" ").upper())
        return self._proc_req(key, cmd, buf)

    def process_at(self, buf: str) -> str:
        return self._cfg.at_result(buf)


class DongleEmu(Dongle):
    _ser = None
    _running = False

    def __init__(self, url: str, cfg):
        super().__init__()
        self._device = YamlDevice(self, cfg)
        self._ser = serial.serial_for_url(url, baudrate=9600)
        self._running = False

    def __del__(self):
        self.close()

    def close(self):
        if self._ser is not None:
            self._ser.close()

    def stop(self):
        self._running = False

    def run(self):
        self._running = True
        buf = bytes()
        # oldtime = time.time()
        while self._running:
            available = self._ser.in_waiting
            if available > 0:
                try:
                    buf = self._rx_buf(buf + self._ser.read(available))
                except serial.SerialException as err:
                    _LOGGER.error(err)
                    break
            time.sleep(1)
            # if time.time() - oldtime > 10:
            #     oldtime = time.time()
            #     self.write_cmd(10, b"\x01\xEE\xFF")
            if self.is_test and available == 0:
                self._running = False

    def _rx_buf(self, buf: bytes) -> bytes:
        """Process input buffer"""
        _LOGGER.debug("RX: %s", buf.hex(" ").upper())
        # max_buf_len = self._device.max_request_size
        while len(buf) > 3:
            if buf[0] == 0xAA:
                end = buf[1] + 3
                # print("end", end)
                if len(buf) < end:
                    # not enought data len
                    break
                self._rx_packet(buf[:end])
                buf = buf[end:]
                continue
            if buf[0] == ord("A") and buf[1] == ord("T") and buf[2] == ord("+"):
                index_r = buf.find(b"\r\n", 3)
                if index_r < 0:
                    # not enought data len
                    break
                self._rx_at(buf[3:index_r].decode("utf-8"))
                buf = buf[index_r + 2 :]
                continue
            _LOGGER.warning("Unknown RX input %s: %s", buf.hex(" "), buf)
            buf = buf[1:]

        return buf

    def _calc_crc(self, data: bytes) -> int:
        crc = 0
        for _, byte in enumerate(data):
            crc = crc + byte
        return crc & 0xFF

    def _rx_packet(self, buf: bytes) -> None:
        _LOGGER.debug("RX: %s", buf.hex(" ").upper())
        if self._calc_crc(buf[: len(buf) - 1]) != buf[len(buf) - 1]:
            _LOGGER.warning("Invalid CRC")
            return
        cmd = buf[2]
        buf = buf[3 : len(buf) - 1]
        if not self._device.process_cmd(cmd, buf):
            _LOGGER.warning(
                "Unknown packet command: %02X, data: %s (%d)",
                cmd,
                buf.hex("."),
                len(buf),
            )

    def _rx_at(self, buf: str):
        _LOGGER.debug("RX: %s", buf)
        res = self._device.process_at(buf)
        if res is None:
            _LOGGER.warning("Unknown AT command: %s", buf)
        _LOGGER.debug("TX: %s", res)
        if not self.is_test:
            self._ser.write(res.encode("utf-8") + b"\r\n")

    def wrap_cmd(self, data: bytes) -> bytes:
        buf = struct.pack("<BB", 0xAA, len(data)) + data
        crc = self._calc_crc(buf)
        buf = buf + struct.pack("<B", crc)
        return buf

    def write_cmd(self, cmd: int, data: bytes):
        """Write command"""
        buf = self.wrap_cmd(struct.pack("<B", cmd) + data)
        _LOGGER.debug("TX: %s", buf.hex(" ").upper())
        if not self.is_test:
            self._ser.write(buf)
            # self._rx_packet(buf)

    def test(self, data: str):
        if self.is_test:
            if data.startswith("AT+"):
                self._ser.write(data.encode("utf-8") + b"\r\n")
            else:
                data = data.replace(" ", "").replace(".", "")
                bin_data = bytes.fromhex(data)
                if bin_data[0] != 0xAA:
                    bin_data = self.wrap_cmd(bin_data)
                self._ser.write(bin_data)

    @property
    def is_test(self) -> bool:
        return self._ser.port.startswith("loop://")


class CustomFormatter(logging.Formatter):
    _grey = "\x1b[38;20m"
    _blue = "\x1b[38;5;39m"
    _yellow = "\x1b[33;20m"
    _red = "\x1b[31;20m"
    _bold_red = "\x1b[31;1m"
    _reset = "\x1b[0m"
    _fmt = "%(asctime)s %(levelname)-7s %(message)s"

    FORMATS = {
        logging.DEBUG: _grey + _fmt + _reset,
        logging.INFO: _blue + _fmt + _reset,
        logging.WARNING: _yellow + _fmt + _reset,
        logging.ERROR: _red + _fmt + _reset,
        logging.CRITICAL: _bold_red + _fmt + _reset,
    }

    def format(self, record: logging.LogRecord) -> str:
        log_fmt = self.FORMATS.get(record.levelno)
        formatter = logging.Formatter(log_fmt)
        return formatter.format(record)


def setup_logging():
    # logging.basicConfig(format="%(asctime)s %(levelname)-7s %(message)s")
    log_handler = logging.StreamHandler()
    log_handler.setFormatter(CustomFormatter())
    _LOGGER.addHandler(log_handler)
    _LOGGER.setLevel(logging.DEBUG)


def main(argv: list[str]):
    if len(argv) <= 1:
        prg = "emu.py" if len(argv) == 0 else argv[0]
        print("Usage:")
        print(f"  {prg} <config> [device]")
        print("Examples:")
        print(f"  {prg} ewh.yaml /dev/cu.usbmodem1443230")
        return

    setup_logging()

    cfg = EmuConfig()
    try:
        cfg.read_config(argv[1])
        emu = DongleEmu(argv[2] if len(argv) == 3 else "loop://?logging=info", cfg)

        def stop(signum, frame):  # pylint: disable=unused-argument
            print("\nStopping...")
            emu.stop()

        signal.signal(signal.SIGINT, stop)
        signal.signal(signal.SIGTERM, stop)

        if emu.is_test:
            cfg.run_tests(emu)
        emu.run()
    except Exception as ex:
        _LOGGER.error(ex)


if __name__ == "__main__":
    main(sys.argv)
