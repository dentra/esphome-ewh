import asyncio
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
    ("device_state", "preset", "action"),
    [
        (EWH.STATE_700W, "0.7 kW", ClimateAction.HEATING),
        (EWH.STATE_1300W, "1.3 kW", ClimateAction.HEATING),
        (EWH.STATE_2000W, "2.0 kW", ClimateAction.HEATING),
        # no frost keeps water just above freezing
        (EWH.STATE_NO_FROST, "No Frost", ClimateAction.IDLE),
    ],
)
async def test_device_state_to_preset(ewh, device_state, preset, action):
    await ewh.wait_state(CLIMATE, lambda s: s.current_temperature == 40)
    ewh.device.state = device_state
    ewh.device.push_state()
    state = await ewh.wait_state(CLIMATE, lambda s: s.mode == ClimateMode.HEAT)
    assert state.custom_preset == preset
    assert state.action == action


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


async def push_temp(node, current: int) -> None:
    node.device.current_temperature = current
    node.device.push_state()
    await node.wait_state(CLIMATE, lambda s: s.current_temperature == current)


async def test_heating_estimation(ewh):
    ewh.device.state = EWH.STATE_2000W
    # cooled down more than idle_temp_drop, heating
    await push_temp(ewh, 40)
    await ewh.wait_state(CLIMATE, lambda s: s.action == ClimateAction.HEATING)
    await ewh.wait_state("power", lambda s: s.state == 2000)
    # target reached
    await push_temp(ewh, 60)
    await ewh.wait_state(CLIMATE, lambda s: s.action == ClimateAction.IDLE)
    await ewh.wait_state("power", lambda s: s.state == 0)
    # cools down within idle range
    await push_temp(ewh, 58)
    assert ewh.states[ewh.key(CLIMATE)].action == ClimateAction.IDLE
    # cooled down more than idle_temp_drop
    await push_temp(ewh, 54)
    await ewh.wait_state(CLIMATE, lambda s: s.action == ClimateAction.HEATING)
    # rises
    await push_temp(ewh, 56)
    assert ewh.states[ewh.key(CLIMATE)].action == ClimateAction.HEATING
    await push_temp(ewh, 60)
    await ewh.wait_state(CLIMATE, lambda s: s.action == ClimateAction.IDLE)


async def test_heating_when_rises_within_idle_range(ewh):
    ewh.device.state = EWH.STATE_700W
    await push_temp(ewh, 60)
    await push_temp(ewh, 57)
    assert ewh.states[ewh.key(CLIMATE)].action == ClimateAction.IDLE
    await push_temp(ewh, 58)
    await ewh.wait_state(CLIMATE, lambda s: s.action == ClimateAction.HEATING)
    await ewh.wait_state("power", lambda s: s.state == 700)


@pytest.mark.parametrize(
    ("device_state", "power"),
    [
        (EWH.STATE_700W, 700),
        (EWH.STATE_1300W, 1300),
        (EWH.STATE_2000W, 2000),
    ],
)
async def test_power_by_mode(ewh, device_state, power):
    ewh.device.state = device_state
    ewh.device.push_state()
    await ewh.wait_state("power", lambda s: s.state == power)


async def test_power_when_off(ewh):
    await ewh.wait_state("power", lambda s: s.state == 0)


def energy_value(node) -> float:
    state = node.states.get(node.key("energy"))
    # energy is restored after reboot, it may be missing on first run only
    return 0.0 if state is None or state.missing_state or math.isnan(state.state) else state.state


async def test_energy(ewh):
    await ewh.wait_state("power", lambda s: s.state == 0)
    initial = energy_value(ewh)
    start = time.monotonic()
    ewh.device.state = EWH.STATE_2000W
    ewh.device.push_state()
    await ewh.wait_state("power", lambda s: s.state == 2000)
    await asyncio.sleep(1.5)
    ewh.device.state = EWH.STATE_OFF
    ewh.device.push_state()
    await ewh.wait_state("power", lambda s: s.state == 0)
    elapsed = time.monotonic() - start
    state = await ewh.wait_state("energy", lambda s: s.state > initial)
    # 2000 W during at least 1.5 s and at most elapsed time, in kWh
    assert 2000 * 1.4 / 3600000 < state.state - initial < 2000 * (elapsed + 0.2) / 3600000


async def test_energy_grows_while_power_is_not_changed(ewh):
    ewh.device.state = EWH.STATE_2000W
    ewh.device.push_state()
    await ewh.wait_state("power", lambda s: s.state == 2000)
    initial = energy_value(ewh)
    await asyncio.sleep(1)
    # periodic device state push with the same power
    ewh.device.push_state()
    await ewh.wait_state("energy", lambda s: s.state > initial)


async def test_timer_is_not_counted(ewh):
    ewh.device.state = EWH.STATE_TIMER
    ewh.device.push_state()
    state = await ewh.wait_state(CLIMATE, lambda s: s.mode == ClimateMode.HEAT)
    assert state.action == ClimateAction.IDLE
    await ewh.wait_state("power", lambda s: s.state == 0)


@pytest.mark.parametrize(("current", "power"), [(5, 700), (10, 0)])
async def test_no_frost_power(ewh, current, power):
    ewh.device.state = EWH.STATE_NO_FROST
    ewh.device.current_temperature = current
    ewh.device.push_state()
    await ewh.wait_state(CLIMATE, lambda s: s.current_temperature == current)
    await ewh.wait_state("power", lambda s: s.state == power)


async def test_heating_when_turned_on_within_idle_range(ewh):
    await push_temp(ewh, 58)
    ewh.device.state = EWH.STATE_700W
    ewh.device.push_state()
    await ewh.wait_state(CLIMATE, lambda s: s.action == ClimateAction.HEATING)


async def test_heating_while_hot_water_is_used(ewh):
    ewh.device.state = EWH.STATE_700W
    await push_temp(ewh, 50)
    await ewh.wait_state(CLIMATE, lambda s: s.action == ClimateAction.HEATING)
    await push_temp(ewh, 58)
    # temperature goes down while heating
    await push_temp(ewh, 57)
    assert ewh.states[ewh.key(CLIMATE)].action == ClimateAction.HEATING


async def test_heating_when_target_raised(ewh):
    ewh.device.state = EWH.STATE_700W
    await push_temp(ewh, 60)
    await ewh.wait_state(CLIMATE, lambda s: s.action == ClimateAction.IDLE)
    ewh.device.target_temperature = 63
    ewh.device.push_state()
    await ewh.wait_state(CLIMATE, lambda s: s.action == ClimateAction.HEATING)


async def test_invalid_target_keeps_estimation(ewh):
    ewh.device.state = EWH.STATE_2000W
    ewh.device.push_state()
    await ewh.wait_state("power", lambda s: s.state == 2000)
    ewh.device.target_temperature = 0
    await push_temp(ewh, 41)
    assert ewh.states[ewh.key(CLIMATE)].action == ClimateAction.HEATING
    assert ewh.states[ewh.key("power")].state == 2000
