import pytest
from aioesphomeapi import ClimateAction, ClimateMode

from rka import BWH

CLIMATE = "water_heater"


@pytest.fixture
async def bwh(node_factory):
    async for node in node_factory("bwh", BWH()):
        # real device pushes its state every 30 seconds, do not wait for it
        node.device.push_state()
        await node.wait_state(CLIMATE, lambda s: s.current_temperature == 40)
        yield node


async def test_initial_state(bwh):
    state = await bwh.wait_state(CLIMATE, lambda s: True)
    assert state.mode == ClimateMode.OFF
    assert state.target_temperature == 60


@pytest.mark.parametrize(
    ("device_state", "preset"),
    [(BWH.STATE_1300W, "1.3 kW"), (BWH.STATE_2000W, "2.0 kW")],
)
async def test_device_state_to_preset(bwh, device_state, preset):
    bwh.device.state = device_state
    bwh.device.push_state()
    state = await bwh.wait_state(CLIMATE, lambda s: s.mode == ClimateMode.HEAT)
    assert state.custom_preset == preset
    assert state.action == ClimateAction.HEATING


async def test_set_preset(bwh):
    start = len(bwh.device.requests)
    bwh.client.climate_command(
        bwh.key(CLIMATE), mode=ClimateMode.HEAT, custom_preset="2.0 kW"
    )
    bwh.device.wait_set(bytes([0x00, BWH.STATE_2000W, 60]), start=start)
    await bwh.wait_state(CLIMATE, lambda s: s.custom_preset == "2.0 kW")


async def test_heat_without_preset_uses_1300w(bwh):
    start = len(bwh.device.requests)
    bwh.client.climate_command(bwh.key(CLIMATE), mode=ClimateMode.HEAT)
    bwh.device.wait_set(bytes([0x00, BWH.STATE_1300W, 60]), start=start)


async def test_turn_off(bwh):
    bwh.device.state = BWH.STATE_2000W
    bwh.device.push_state()
    await bwh.wait_state(CLIMATE, lambda s: s.mode == ClimateMode.HEAT)
    start = len(bwh.device.requests)
    bwh.client.climate_command(bwh.key(CLIMATE), mode=ClimateMode.OFF)
    bwh.device.wait_set(bytes([0x00, BWH.STATE_OFF, 60]), start=start)
    await bwh.wait_state(CLIMATE, lambda s: s.mode == ClimateMode.OFF)


async def test_on_state_trigger(bwh):
    bwh.device.error = 3
    bwh.device.push_state()
    await bwh.wait_state("error_code", lambda s: s.state == 3)


async def test_on_state_trigger_fires_once_per_state(bwh):
    first = (await bwh.wait_state("on_state_count", lambda s: s.state > 0)).state
    bwh.device.error = 5
    bwh.device.push_state()
    await bwh.wait_state("error_code", lambda s: s.state == 5)
    assert bwh.states[bwh.key("on_state_count")].state == first + 1
