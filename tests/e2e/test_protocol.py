"""RKA framing robustness, checked on EWH."""

import asyncio

import pytest

from rka import EWH, wrap

CLIMATE = "water_heater"


@pytest.fixture
async def ewh(node_factory):
    async for node in node_factory("ewh", EWH()):
        node.device.push_state()
        await node.wait_state(CLIMATE, lambda s: s.current_temperature == 40)
        yield node


def frame(node, current_temperature: int) -> bytes:
    node.device.current_temperature = current_temperature
    return node.device.state_frame()


async def wait_current(node, value: int) -> None:
    await node.wait_state(CLIMATE, lambda s: s.current_temperature == value)


async def test_fragmented_frame(ewh):
    for byte in frame(ewh, 41):
        ewh.device.write_raw(bytes([byte]))
        await asyncio.sleep(0.03)
    await wait_current(ewh, 41)


async def test_concatenated_frames(ewh):
    ewh.device.write_raw(frame(ewh, 41) + frame(ewh, 42))
    await wait_current(ewh, 42)


async def test_garbage_before_frame(ewh):
    ewh.device.write_raw(b"\x00\x13\x37" + frame(ewh, 41))
    await wait_current(ewh, 41)


async def test_invalid_crc_is_ignored(ewh):
    bad = bytearray(frame(ewh, 50))
    bad[-1] ^= 0xFF
    start = len(ewh.logs)
    ewh.device.write_raw(bytes(bad))
    await ewh.wait_log("Invalid CRC", start=start)
    await asyncio.sleep(0.2)
    assert ewh.states[ewh.key(CLIMATE)].current_temperature == 40
    ewh.device.write_raw(frame(ewh, 41))
    await wait_current(ewh, 41)


async def test_oversized_frame_is_skipped(ewh):
    start = len(ewh.logs)
    ewh.device.write_raw(b"\xaa\xff")
    await ewh.wait_log("Frame size is larger", start=start)
    ewh.device.write_raw(frame(ewh, 41))
    await wait_current(ewh, 41)


async def test_invalid_state_size(ewh):
    start = len(ewh.logs)
    ewh.device.write_raw(wrap(0x09, b"\x01\x02"))
    await ewh.wait_log("Invalid packet size", start=start)
    ewh.device.write_raw(frame(ewh, 41))
    await wait_current(ewh, 41)


@pytest.mark.parametrize(
    ("code", "message"),
    [(1, "invalid CRC"), (2, "invalid command"), (7, "code 07")],
)
async def test_error_packet(ewh, code, message):
    start = len(ewh.logs)
    ewh.device.write_raw(wrap(0x05, bytes([code])))
    await ewh.wait_log(message, start=start)
    ewh.device.write_raw(frame(ewh, 41))
    await wait_current(ewh, 41)


class WrongTypeEWH(EWH):
    DEV_TYPE = b"\x00\x00\x00\x00\x00\x04"


async def test_wrong_device_type(node_factory):
    async for node in node_factory("ewh", WrongTypeEWH()):
        await node.wait_log("rka_dev_type_t.type, actual 0400, expected 1100")
