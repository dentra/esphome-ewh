import time

import pytest

from rka import EHU


@pytest.fixture
async def ehu(node_factory):
    async for node in node_factory("ehu", EHU()):
        # initial state is requested by polling right after setup
        await node.wait_state("temperature", lambda s: s.state == 23)
        yield node


def push(node, **fields) -> None:
    for name, value in fields.items():
        node.device[getattr(EHU, name.upper())] = value
    node.device.push_state()


async def test_initial_state(ehu):
    assert (await ehu.wait_state("humidity", lambda s: True)).state == 45
    assert not (await ehu.wait_state("power", lambda s: True)).state
    assert (await ehu.wait_state("water", lambda s: True)).state
    assert (await ehu.wait_state("target_humidity", lambda s: True)).state == 50
    assert (await ehu.wait_state("led_brightness", lambda s: True)).state == 66
    assert (await ehu.wait_state("preset", lambda s: True)).state == "Manual"
    assert (await ehu.wait_state("led_preset", lambda s: True)).state == "None"


async def test_water_tank_empty(ehu):
    push(ehu, water_tank_empty=1)
    await ehu.wait_state("water", lambda s: not s.state)


async def test_sensors_follow_device(ehu):
    push(ehu, temperature=25, humidity=60)
    await ehu.wait_state("temperature", lambda s: s.state == 25)
    await ehu.wait_state("humidity", lambda s: s.state == 60)


@pytest.mark.parametrize(
    ("object_id", "field", "value"),
    [
        ("power", "power", 1),
        ("ionizer", "ionizer", 1),
        ("lock", "lock", 1),
        ("mute", "mute", 1),
        ("warm_mist", "water_flags", EHU.WATER_WARM_MIST),
        ("uv", "water_flags", EHU.WATER_UV),
        ("led_top", "led_mode", EHU.LED_MODE_TOP),
        ("led_bottom", "led_mode", EHU.LED_MODE_BOTTOM),
    ],
)
async def test_switch_state_from_device(ehu, object_id, field, value):
    await ehu.wait_state(object_id, lambda s: not s.state)
    push(ehu, **{field: value})
    await ehu.wait_state(object_id, lambda s: s.state)


@pytest.mark.parametrize(
    ("object_id", "cmd"),
    [
        ("power", EHU.CMD_POWER),
        ("ionizer", EHU.CMD_IONIZER),
        ("lock", EHU.CMD_LOCK),
        ("mute", EHU.CMD_MUTE),
    ],
)
async def test_switch_command(ehu, object_id, cmd):
    start = len(ehu.device.requests)
    ehu.client.switch_command(ehu.key(object_id), True)
    ehu.device.wait_cmd(cmd, b"\x01", start=start)


@pytest.mark.parametrize(
    ("object_id", "cmd", "current", "expected"),
    [
        # turning one flag on keeps the other one
        ("warm_mist", EHU.CMD_WARM_MIST_UV, ("water_flags", EHU.WATER_UV), 0x03),
        ("uv", EHU.CMD_WARM_MIST_UV, ("water_flags", EHU.WATER_WARM_MIST), 0x03),
        ("led_top", EHU.CMD_LED_MODE, ("led_mode", EHU.LED_MODE_BOTTOM), 0x03),
        ("led_bottom", EHU.CMD_LED_MODE, ("led_mode", EHU.LED_MODE_TOP), 0x03),
    ],
)
async def test_depended_switch_command(ehu, object_id, cmd, current, expected):
    other = {
        "warm_mist": "uv",
        "uv": "warm_mist",
        "led_top": "led_bottom",
        "led_bottom": "led_top",
    }[object_id]
    push(ehu, **dict([current]))
    await ehu.wait_state(other, lambda s: s.state)
    start = len(ehu.device.requests)
    ehu.client.switch_command(ehu.key(object_id), True)
    ehu.device.wait_cmd(cmd, bytes([expected]), start=start)


async def test_fan_state_from_device(ehu):
    push(ehu, power=1, fan_speed=3, preset=EHU.PRESET_NIGHT)
    state = await ehu.wait_state("fan", lambda s: s.state)
    assert state.speed_level == 3
    assert state.preset_mode == "Night"
    await ehu.wait_state("preset", lambda s: s.state == "Night")
    await ehu.wait_state("speed", lambda s: s.state == 3)
    await ehu.wait_state("humidification", lambda s: s.state)


async def test_fan_speed_command_restores_target_humidity(ehu):
    await ehu.wait_state("target_humidity", lambda s: s.state == 50)
    start = len(ehu.device.requests)
    ehu.client.fan_command(ehu.key("fan"), speed_level=2)
    ehu.device.wait_cmd(EHU.CMD_SPEED, b"\x02", start=start)
    ehu.device.wait_cmd(EHU.CMD_HUMIDITY, bytes([50]), start=start)


async def test_fan_preset_command(ehu):
    start = len(ehu.device.requests)
    ehu.client.fan_command(ehu.key("fan"), preset_mode="Auto")
    ehu.device.wait_cmd(EHU.CMD_PRESET, bytes([EHU.PRESET_AUTO]), start=start)


async def test_preset_select_command(ehu):
    start = len(ehu.device.requests)
    ehu.client.select_command(ehu.key("preset"), "Night")
    ehu.device.wait_cmd(EHU.CMD_PRESET, bytes([EHU.PRESET_NIGHT]), start=start)


async def test_target_humidity_command(ehu):
    start = len(ehu.device.requests)
    ehu.client.number_command(ehu.key("target_humidity"), 70)
    ehu.device.wait_cmd(EHU.CMD_HUMIDITY, bytes([70]), start=start)


async def test_led_brightness_command(ehu):
    start = len(ehu.device.requests)
    ehu.client.number_command(ehu.key("led_brightness"), 99)
    ehu.device.wait_cmd(EHU.CMD_LED_BRIGHTNESS, bytes([99]), start=start)


async def test_led_brightness_ignored_in_random_preset(ehu):
    push(ehu, led_preset=EHU.LED_PRESET_RANDOM)
    await ehu.wait_state("led_preset", lambda s: s.state == "Random")
    start = len(ehu.device.requests)
    ehu.client.number_command(ehu.key("led_brightness"), 99)
    with pytest.raises(TimeoutError):
        ehu.device.wait_request(
            lambda c, d: c == EHU.CMD_LED_BRIGHTNESS, timeout=1, start=start
        )


async def test_led_preset_when_power_off(ehu):
    # when off, only Random can turn LEDs on, then color can be changed
    start = len(ehu.device.requests)
    ehu.client.select_command(ehu.key("led_preset"), "Blue")
    ehu.device.wait_cmd(EHU.CMD_LED_PRESET, bytes([EHU.LED_PRESET_RANDOM]), start=start)
    ehu.device.wait_cmd(EHU.CMD_LED_PRESET, bytes([EHU.LED_PRESET_BLUE]), start=start)


async def test_led_preset_when_power_on(ehu):
    push(ehu, power=1)
    await ehu.wait_state("power", lambda s: s.state)
    start = len(ehu.device.requests)
    ehu.client.select_command(ehu.key("led_preset"), "Blue")
    ehu.device.wait_cmd(EHU.CMD_LED_PRESET, bytes([EHU.LED_PRESET_BLUE]), start=start)
    assert (EHU.CMD_LED_PRESET, bytes([EHU.LED_PRESET_RANDOM])) not in ehu.device.requests[start:]


@pytest.mark.parametrize("same", [None, "hours", "minutes"])
async def test_clock_sync(ehu, same):
    now = time.localtime()
    hours = now.tm_hour if same == "hours" else (now.tm_hour + 1) % 24
    minutes = now.tm_min if same == "minutes" else (now.tm_min + 30) % 60
    start = len(ehu.device.requests)
    push(ehu, clock_hours=hours, clock_minutes=minutes)
    ehu.device.wait_request(lambda c, d: c == EHU.CMD_CLOCK, start=start)


async def test_on_state_trigger_on_device_push(ehu):
    push(ehu, humidity=77)
    await ehu.wait_state("raw_humidity", lambda s: s.state == 77)
