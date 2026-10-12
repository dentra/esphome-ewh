"""Rusklimat (RKA) UART protocol emulator for E2E tests."""

import os
import struct
import select
import threading
import time
from collections.abc import Callable

PACKET_REQ_DEV_TYPE = 0x01
PACKET_REQ_SAVE_DATA = 0x06
PACKET_REQ_LOAD_DATA = 0x07
PACKET_REQ_STATE = 0x08
PACKET_REQ_SET_COMMAND = 0x0A
PACKET_CMD_STATE = 0x09
RSP_MASK = 0x80


def crc(data: bytes) -> int:
    return sum(data) & 0xFF


def wrap(cmd: int, data: bytes = b"") -> bytes:
    buf = bytes([0xAA, len(data) + 1, cmd]) + data
    return buf + bytes([crc(buf)])


class Device:
    """Base device: framing, request log and common RKA commands."""

    DEV_TYPE = b"\x00\x00\x00\x00\x00\x00"

    def __init__(self) -> None:
        self.storage = bytes(16)
        self.requests: list[tuple[int, bytes]] = []
        self._fd: int | None = None
        self._buf = b""
        self._lock = threading.Lock()
        self._cond = threading.Condition(self._lock)
        self._thread: threading.Thread | None = None
        self._running = False

    def start(self, fd: int) -> None:
        self._fd = fd
        self._running = True
        self._thread = threading.Thread(target=self._run, daemon=True)
        self._thread.start()

    def stop(self) -> None:
        self._running = False
        if self._thread is not None:
            self._thread.join(timeout=2)

    def _run(self) -> None:
        while self._running:
            ready, _, _ = select.select([self._fd], [], [], 0.1)
            if not ready:
                continue
            try:
                data = os.read(self._fd, 1024)
            except OSError:
                # slave side closed
                time.sleep(0.05)
                continue
            self._buf = self._process(self._buf + data)

    def _process(self, buf: bytes) -> bytes:
        while len(buf) >= 4:
            if buf[0] != 0xAA:
                buf = buf[1:]
                continue
            end = buf[1] + 3
            if len(buf) < end:
                break
            frame, buf = buf[:end], buf[end:]
            if crc(frame[:-1]) != frame[-1]:
                continue
            cmd, data = frame[2], frame[3:-1]
            with self._cond:
                self.requests.append((cmd, data))
                self._cond.notify_all()
                self.handle(cmd, data)
        return buf

    def write(self, cmd: int, data: bytes = b"") -> None:
        os.write(self._fd, wrap(cmd, data))

    def write_raw(self, data: bytes) -> None:
        with self._lock:
            os.write(self._fd, data)

    def state_frame(self) -> bytes:
        return wrap(PACKET_CMD_STATE, self.state_bytes())

    def handle(self, cmd: int, data: bytes) -> None:
        if cmd == PACKET_REQ_DEV_TYPE:
            self.write(PACKET_REQ_DEV_TYPE | RSP_MASK, self.DEV_TYPE)
        elif cmd == PACKET_REQ_SAVE_DATA:
            self.storage = data
            self.write(PACKET_REQ_SAVE_DATA | RSP_MASK, b"\x01")
        elif cmd == PACKET_REQ_LOAD_DATA:
            self.write(PACKET_REQ_LOAD_DATA | RSP_MASK, self.storage)
        elif cmd == PACKET_REQ_STATE:
            self.write(PACKET_REQ_STATE | RSP_MASK, self.state_bytes())
        else:
            self.command(cmd, data)

    def push_state(self) -> None:
        """Send unsolicited state, like the real device does periodically."""
        with self._lock:
            self.write(PACKET_CMD_STATE, self.state_bytes())

    def state_bytes(self) -> bytes:
        raise NotImplementedError

    def command(self, cmd: int, data: bytes) -> None:
        raise NotImplementedError

    def wait_request(
        self, match: Callable[[int, bytes], bool], timeout: float = 5.0, start: int = 0
    ) -> tuple[int, bytes]:
        """Wait for a request matching predicate, searching from index `start`."""
        deadline = time.monotonic() + timeout
        with self._cond:
            while True:
                for cmd, data in self.requests[start:]:
                    if match(cmd, data):
                        return cmd, data
                left = deadline - time.monotonic()
                if left <= 0:
                    raise TimeoutError(f"request not received, got: {self.dump()}")
                self._cond.wait(left)

    def wait_cmd(
        self, cmd: int, payload: bytes, timeout: float = 5.0, start: int = 0
    ) -> None:
        self.wait_request(lambda c, d: c == cmd and d == payload, timeout, start)

    def wait_set(self, payload: bytes, timeout: float = 5.0, start: int = 0) -> None:
        self.wait_cmd(PACKET_REQ_SET_COMMAND, payload, timeout, start)

    def dump(self) -> str:
        return ", ".join(f"{c:02X}:{d.hex('.')}" for c, d in self.requests)


class EWH(Device):
    """Electrolux water heater."""

    DEV_TYPE = b"\x00\x00\x00\x00\x00\x11"

    STATE_OFF = 0
    STATE_700W = 1
    STATE_1300W = 2
    STATE_2000W = 3
    STATE_TIMER = 4
    STATE_NO_FROST = 5

    # set mode value -> state value
    MODE_TO_STATE = {0: 0, 1: 1, 2: 2, 3: 3, 4: 5}

    def __init__(self) -> None:
        super().__init__()
        self.state = self.STATE_OFF
        self.current_temperature = 40
        self.target_temperature = 60
        self.clock = (12, 0)
        self.timer = (0, 0)
        self.error = 0
        self.bst = 0

    def state_bytes(self) -> bytes:
        return bytes(
            [
                self.state,
                self.current_temperature,
                self.target_temperature,
                *self.clock,
                *self.timer,
                self.error,
                self.bst,
            ]
        )

    def command(self, cmd: int, data: bytes) -> None:
        if cmd != PACKET_REQ_SET_COMMAND or not data:
            return
        operation, data = data[0], data[1:]
        if operation == 0:
            self.state = self.MODE_TO_STATE.get(data[0], data[0])
            self.target_temperature = data[1]
        elif operation == 1:
            self.clock = (data[0], data[1])
        elif operation == 2:
            self.timer = (data[0], data[1])
            self.target_temperature = data[3]
            self.state = self.STATE_TIMER
        elif operation == 3:
            self.bst = data[0]


class EHU(Device):
    """Electrolux humidifier."""

    DEV_TYPE = b"\x00\x00\x00\x00\x00\x07"
    STATE_SIZE = 49

    # ehu_state_t field offsets
    POWER = 0
    PRESET = 1
    WATER_FLAGS = 10
    IONIZER = 11
    MUTE = 12
    LOCK = 14
    LED_PRESET = 16
    LED_BRIGHTNESS = 17
    LED_MODE = 18
    TARGET_HUMIDITY = 20
    FAN_SPEED = 21
    CLOCK_HOURS = 22
    CLOCK_MINUTES = 23
    TEMPERATURE = 24
    HUMIDITY = 25
    WATER_TANK_EMPTY = 26

    WATER_WARM_MIST = 1 << 0
    WATER_UV = 1 << 1
    LED_MODE_BOTTOM = 1 << 0
    LED_MODE_TOP = 1 << 1

    PRESET_AUTO = 0x01
    PRESET_NIGHT = 0x03
    PRESET_MANUAL = 0x0F
    LED_PRESET_OFF = 0
    LED_PRESET_RANDOM = 1
    LED_PRESET_BLUE = 2

    CMD_POWER = 0x0A
    CMD_PRESET = 0x0B
    CMD_WARM_MIST_UV = 0x0F
    CMD_IONIZER = 0x11
    CMD_MUTE = 0x12
    CMD_LOCK = 0x14
    CMD_LED_PRESET = 0x16
    CMD_LED_BRIGHTNESS = 0x17
    CMD_HUMIDITY = 0x19
    CMD_SPEED = 0x1A
    CMD_CLOCK = 0x1B
    CMD_LED_MODE = 0x23

    # command -> state field offset
    CMD_FIELDS = {
        CMD_POWER: POWER,
        CMD_PRESET: PRESET,
        CMD_WARM_MIST_UV: WATER_FLAGS,
        CMD_IONIZER: IONIZER,
        CMD_MUTE: MUTE,
        CMD_LOCK: LOCK,
        CMD_LED_PRESET: LED_PRESET,
        CMD_LED_BRIGHTNESS: LED_BRIGHTNESS,
        CMD_HUMIDITY: TARGET_HUMIDITY,
        CMD_SPEED: FAN_SPEED,
        CMD_LED_MODE: LED_MODE,
    }

    def __init__(self) -> None:
        super().__init__()
        self.st = bytearray(self.STATE_SIZE)
        self.st[self.PRESET] = self.PRESET_MANUAL
        self.st[self.LED_BRIGHTNESS] = 66
        self.st[self.TARGET_HUMIDITY] = 50
        self.st[self.FAN_SPEED] = 1
        self.st[self.TEMPERATURE] = 23
        self.st[self.HUMIDITY] = 45

    def __getitem__(self, offset: int) -> int:
        return self.st[offset]

    def __setitem__(self, offset: int, value: int) -> None:
        self.st[offset] = value

    def state_bytes(self) -> bytes:
        return bytes(self.st)

    def command(self, cmd: int, data: bytes) -> None:
        if cmd == self.CMD_CLOCK:
            self.st[self.CLOCK_HOURS], self.st[self.CLOCK_MINUTES] = data[0], data[1]
        elif cmd in self.CMD_FIELDS and data:
            self.st[self.CMD_FIELDS[cmd]] = data[0]


class BWH(Device):
    """Ballu water heater."""

    DEV_TYPE = b"\x00\x00\x00\x00\x00\x04"

    STATE_OFF = 0
    STATE_1300W = 1
    STATE_2000W = 2

    def __init__(self) -> None:
        super().__init__()
        self.state = self.STATE_OFF
        self.current_temperature = 40
        self.target_temperature = 60
        self.error = 0

    def state_bytes(self) -> bytes:
        return bytes(
            [self.state, self.current_temperature, self.target_temperature]
            + [0, 0, 0, 0]
            + [self.error]
        )

    def command(self, cmd: int, data: bytes) -> None:
        if cmd == PACKET_REQ_SET_COMMAND and len(data) == 3 and data[0] == 0:
            # mode values are the same as state values
            self.state, self.target_temperature = data[1], data[2]


class ETS(Device):
    """Electrolux thermostat."""

    DEV_TYPE = b"\x00\x00\x00\x00\x01\x05"
    UNCHANGED = 0x7F
    FLOOR_NOT_CONNECTED = -95

    def __init__(self) -> None:
        super().__init__()
        self.on = False
        # temperatures in tenths of degree
        self.target_temp = 250
        self.air_temp = 276
        self.floor_temp = self.FLOOR_NOT_CONNECTED
        self.unk0c = 0x01C2

    def state_bytes(self) -> bytes:
        return struct.pack(
            ">BBhhHhBBHBBBBBB",
            int(self.on),
            0,
            self.target_temp,
            self.air_temp,
            0,
            self.floor_temp,
            1,
            2,
            self.unk0c,
            0,
            0,
            30,
            0,
            1,
            1,
        )

    def push_state(self) -> None:
        """ETS does not push state, send it as unsolicited state response."""
        with self._lock:
            self.write(PACKET_REQ_STATE | RSP_MASK, self.state_bytes())

    def command(self, cmd: int, data: bytes) -> None:
        if cmd != PACKET_REQ_SET_COMMAND or len(data) != 20:
            return
        state, _, target_temp = struct.unpack(">BBh", data[:4])
        if state != self.UNCHANGED:
            self.on = state != 0
        self.target_temp = target_temp
        self.write(PACKET_REQ_SET_COMMAND | RSP_MASK, self.state_bytes())
