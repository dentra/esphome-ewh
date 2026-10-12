import struct

import pytest
from aioesphomeapi import ClimateMode

from rka import ETS, PACKET_REQ_SET_COMMAND

CLIMATE = "thermostat"


@pytest.fixture
async def ets(node_factory):
    async for node in node_factory("ets", ETS()):
        # initial state is requested by polling right after setup
        await node.wait_state(CLIMATE, lambda s: round(s.current_temperature, 1) == 27.6)
        yield node


def wait_set_mode(node, start: int) -> tuple[int, int]:
    """Wait for set mode request, return (state, target temperature in tenths)."""
    _, data = node.device.wait_request(
        lambda c, d: c == PACKET_REQ_SET_COMMAND and len(d) == 20, start=start
    )
    state, _, target_temp = struct.unpack(">BBh", data[:4])
    return state, target_temp


async def test_initial_state(ets):
    state = await ets.wait_state(CLIMATE, lambda s: True)
    assert state.mode == ClimateMode.OFF
    assert round(state.target_temperature, 1) == 25.0


async def test_floor_sensor_not_connected(ets):
    await ets.wait_state("raw_brightness", lambda s: s.state == 30)
    state = await ets.wait_state("floor_temperature", lambda s: True)
    assert state.missing_state, state


async def test_floor_sensor(ets):
    ets.device.floor_temp = 215
    ets.device.push_state()
    await ets.wait_state("floor_temperature", lambda s: round(s.state, 1) == 21.5)


async def test_negative_temperature(ets):
    ets.device.air_temp = -52
    ets.device.floor_temp = -12
    ets.device.push_state()
    await ets.wait_state(CLIMATE, lambda s: round(s.current_temperature, 1) == -5.2)
    await ets.wait_state("floor_temperature", lambda s: round(s.state, 1) == -1.2)


async def test_turn_on(ets):
    start = len(ets.device.requests)
    ets.client.climate_command(ets.key(CLIMATE), mode=ClimateMode.HEAT)
    assert wait_set_mode(ets, start) == (1, 250)
    await ets.wait_state(CLIMATE, lambda s: s.mode == ClimateMode.HEAT)


@pytest.mark.parametrize("target", [27.6, 21.3, 18.7, 5.1, 44.9])
async def test_set_target_temperature_keeps_state(ets, target):
    start = len(ets.device.requests)
    ets.client.climate_command(ets.key(CLIMATE), target_temperature=target)
    assert wait_set_mode(ets, start) == (ETS.UNCHANGED, round(target * 10))
    await ets.wait_state(CLIMATE, lambda s: round(s.target_temperature, 1) == target)


async def test_on_state_trigger(ets):
    await ets.wait_state("raw_brightness", lambda s: s.state == 30)


async def test_negative_air_temperature_in_command(ets):
    ets.device.air_temp = -52
    ets.device.push_state()
    await ets.wait_state(CLIMATE, lambda s: round(s.current_temperature, 1) == -5.2)
    start = len(ets.device.requests)
    ets.client.climate_command(ets.key(CLIMATE), target_temperature=20)
    _, data = ets.device.wait_request(
        lambda c, d: c == PACKET_REQ_SET_COMMAND and len(d) == 20, start=start
    )
    assert struct.unpack(">h", data[6:8])[0] == -52
