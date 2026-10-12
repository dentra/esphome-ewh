import math
import time

import pytest
from aioesphomeapi import ClimateAction, ClimateMode

from rka import EWH

CLIMATE = "water_heater"
BST = "bst"


@pytest.fixture
async def ewh(node_factory):
    async for node in node_factory("ewh", EWH()):
        # real device pushes its state every 30 seconds, do not wait for it
        node.device.push_state()
        yield node


async def test_initial_state(ewh):
    state = await ewh.wait_state(CLIMATE, lambda s: s.current_temperature == 40)
    assert state.mode == ClimateMode.OFF
    assert state.action == ClimateAction.OFF
    assert state.target_temperature == 60


@pytest.mark.parametrize(
    ("device_state", "preset"),
    [
        (EWH.STATE_700W, "0.7 kW"),
        (EWH.STATE_1300W, "1.3 kW"),
        (EWH.STATE_2000W, "2.0 kW"),
        (EWH.STATE_NO_FROST, "No Frost"),
    ],
)
async def test_device_state_to_preset(ewh, device_state, preset):
    await ewh.wait_state(CLIMATE, lambda s: s.current_temperature == 40)
    ewh.device.state = device_state
    ewh.device.push_state()
    state = await ewh.wait_state(CLIMATE, lambda s: s.mode == ClimateMode.HEAT)
    assert state.custom_preset == preset
    assert state.action == ClimateAction.HEATING


async def test_idle_when_target_reached(ewh):
    await ewh.wait_state(CLIMATE, lambda s: s.current_temperature == 40)
    ewh.device.state = EWH.STATE_700W
    ewh.device.current_temperature = 60
    ewh.device.push_state()
    state = await ewh.wait_state(CLIMATE, lambda s: s.mode == ClimateMode.HEAT)
    assert state.action == ClimateAction.IDLE


async def test_set_preset_and_temperature(ewh):
    await ewh.wait_state(CLIMATE, lambda s: s.current_temperature == 40)
    start = len(ewh.device.requests)
    ewh.client.climate_command(
        ewh.key(CLIMATE),
        mode=ClimateMode.HEAT,
        target_temperature=55,
        custom_preset="1.3 kW",
    )
    ewh.device.wait_set(bytes([0x00, 0x02, 55]), start=start)
    state = await ewh.wait_state(CLIMATE, lambda s: s.custom_preset == "1.3 kW")
    assert state.mode == ClimateMode.HEAT
    assert state.target_temperature == 55


async def test_turn_off(ewh):
    await ewh.wait_state(CLIMATE, lambda s: s.current_temperature == 40)
    ewh.device.state = EWH.STATE_2000W
    ewh.device.push_state()
    await ewh.wait_state(CLIMATE, lambda s: s.mode == ClimateMode.HEAT)
    start = len(ewh.device.requests)
    ewh.client.climate_command(ewh.key(CLIMATE), mode=ClimateMode.OFF)
    ewh.device.wait_set(bytes([0x00, 0x00, 60]), start=start)
    state = await ewh.wait_state(CLIMATE, lambda s: s.mode == ClimateMode.OFF)
    assert state.action == ClimateAction.OFF


async def test_temperature_out_of_range(ewh):
    await ewh.wait_state(CLIMATE, lambda s: s.current_temperature == 40)
    ewh.device.target_temperature = 90
    ewh.device.current_temperature = 41
    ewh.device.push_state()
    state = await ewh.wait_state(CLIMATE, lambda s: s.current_temperature == 41)
    assert math.isnan(state.target_temperature)


async def test_bst_switch(ewh):
    await ewh.wait_state(BST, lambda s: not s.state)
    start = len(ewh.device.requests)
    ewh.client.switch_command(ewh.key(BST), True)
    ewh.device.wait_set(bytes([0x03, 0x01]), start=start)
    # BST does not request state after command, switch is updated by the next device push
    ewh.device.push_state()
    await ewh.wait_state(BST, lambda s: s.state)


async def test_error_code(ewh):
    await ewh.wait_state("error_code", lambda s: s.state == 0)
    ewh.device.error = 1
    ewh.device.push_state()
    await ewh.wait_state("error_code", lambda s: s.state == 1)


async def test_on_state_trigger(ewh):
    ewh.device.timer = (6, 30)
    ewh.device.push_state()
    await ewh.wait_state("timer", lambda s: s.state == "06:30")
    await ewh.wait_state("clock", lambda s: len(s.state) == 5)


@pytest.mark.parametrize("same", [None, "hours", "minutes"])
async def test_clock_sync(ewh, same):
    now = time.localtime()
    hours = now.tm_hour if same == "hours" else (now.tm_hour + 1) % 24
    minutes = now.tm_min if same == "minutes" else (now.tm_min + 30) % 60
    start = len(ewh.device.requests)
    ewh.device.clock = (hours, minutes)
    ewh.device.push_state()
    _, data = ewh.device.wait_request(lambda c, d: c == 0x0A and d[0] == 0x01, start=start)
    assert data[1] == time.localtime().tm_hour


async def test_timer_state(ewh):
    await ewh.wait_state(CLIMATE, lambda s: s.current_temperature == 40)
    ewh.device.state = EWH.STATE_700W
    ewh.device.push_state()
    await ewh.wait_state(CLIMATE, lambda s: s.custom_preset == "0.7 kW")
    ewh.device.state = EWH.STATE_TIMER
    ewh.device.target_temperature = 70
    ewh.device.push_state()
    state = await ewh.wait_state(CLIMATE, lambda s: s.target_temperature == 70)
    assert state.mode == ClimateMode.HEAT
    # Timer is not a supported preset, so preset is cleared
    assert state.custom_preset == ""


async def test_change_preset_when_off(ewh):
    await ewh.wait_state(CLIMATE, lambda s: s.mode == ClimateMode.OFF)
    ewh.client.climate_command(ewh.key(CLIMATE), custom_preset="2.0 kW")
    await ewh.wait_state(CLIMATE, lambda s: s.custom_preset == "2.0 kW")


async def test_clock_lag_does_not_resync(ewh):
    await ewh.wait_state(CLIMATE, lambda s: s.current_temperature == 40)
    # device clock lags up to one minute after sync
    lag = time.localtime(time.time() - 60)
    ewh.device.clock = (lag.tm_hour, lag.tm_min)
    start = len(ewh.device.requests)
    ewh.device.push_state()
    with pytest.raises(TimeoutError):
        ewh.device.wait_request(lambda c, d: c == 0x0A and d[0] == 0x01, timeout=1, start=start)
