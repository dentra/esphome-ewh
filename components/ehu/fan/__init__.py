import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor, fan, sensor, switch
from esphome.const import (
    CONF_HUMIDITY,
    CONF_ID,
    CONF_NAME,
    CONF_TEMPERATURE,
    DEVICE_CLASS_HUMIDITY,
    DEVICE_CLASS_MOISTURE,
    DEVICE_CLASS_TEMPERATURE,
    ENTITY_CATEGORY_CONFIG,
    STATE_CLASS_MEASUREMENT,
    UNIT_CELSIUS,
    UNIT_PERCENT,
)

from .. import EHU_COMPONENT_SCHEMA, EHUComponent, ehu_ns, new_ehu

AUTO_LOAD = ["ehu", "fan", "sensor", "binary_sensor", "switch"]

CONF_WARM_MIST="warm_mist"
CONF_UV="uv"
CONF_IONIZER="ionizer"
CONF_LOCK="lock"
CONF_SOUND="sound"
CONF_WATER="water"

EHUFan = ehu_ns.class_("EHUFan", EHUComponent)
EHUSwitch= ehu_ns.class_("EHUSwitch", switch.Switch, cg.Component)
EHUWarmMistSwitch= ehu_ns.class_("EHUWarmMistSwitch", switch.Switch, cg.Component)
EHUUVSwitch=ehu_ns.class_("EHUUVSwitch", switch.Switch, cg.Component)

EHUPacketType=ehu_ns.enum("ehu_packet_type_t", is_class=True)


CONFIG_SCHEMA = fan.FAN_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(EHUFan),
        cv.Optional(CONF_TEMPERATURE): cv.maybe_simple_value(
            sensor.sensor_schema(
                unit_of_measurement=UNIT_CELSIUS,
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_TEMPERATURE,
                state_class=STATE_CLASS_MEASUREMENT
            ),
            key=CONF_NAME,
        ),
        cv.Optional(CONF_HUMIDITY):  cv.maybe_simple_value(
            sensor.sensor_schema(
                unit_of_measurement=UNIT_PERCENT,
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_HUMIDITY,
                state_class=STATE_CLASS_MEASUREMENT
            ),
            key=CONF_NAME,
        ),
        cv.Optional(CONF_WARM_MIST): cv.maybe_simple_value(
            switch.switch_schema(
                EHUWarmMistSwitch,
                entity_category=ENTITY_CATEGORY_CONFIG,
                block_inverted=True,
            ),
            key=CONF_NAME,
        ),
        cv.Optional(CONF_UV): cv.maybe_simple_value(
            switch.switch_schema(
                EHUUVSwitch,
                entity_category=ENTITY_CATEGORY_CONFIG,
                block_inverted=True,
            ),
            key=CONF_NAME,
        ),
        cv.Optional(CONF_IONIZER): cv.maybe_simple_value(
            switch.switch_schema(
                EHUSwitch.template(EHUPacketType.PACKET_REQ_SET_IONIZER),
                entity_category=ENTITY_CATEGORY_CONFIG,
                block_inverted=True,
            ),
            key=CONF_NAME,
        ),
        cv.Optional(CONF_LOCK): cv.maybe_simple_value(
            switch.switch_schema(
                EHUSwitch.template(EHUPacketType.PACKET_REQ_SET_LOCK),
                entity_category=ENTITY_CATEGORY_CONFIG,
                block_inverted=True,
            ),
            key=CONF_NAME,
        ),
        cv.Optional(CONF_SOUND): cv.maybe_simple_value(
            switch.switch_schema(
                EHUSwitch.template(EHUPacketType.PACKET_REQ_SET_SOUND),
                entity_category=ENTITY_CATEGORY_CONFIG,
                block_inverted=True,
            ),
            key=CONF_NAME,
        ),
        cv.Optional(CONF_WATER): cv.maybe_simple_value(
            binary_sensor.binary_sensor_schema(
                device_class=DEVICE_CLASS_MOISTURE
            ),
            key=CONF_NAME,
        ),
    }
).extend(EHU_COMPONENT_SCHEMA)

async def setup_sensor(config, key, fn):
    if conf := config.get(key):
        sens = await sensor.new_sensor(conf)
        cg.add(fn(sens))

async def setup_switch(config, key, fn):
    if key in config:
        fan = await cg.get_variable(config[CONF_ID])
        var = await switch.new_switch(config[key], fan)
        await cg.register_component(var, config[key])
        cg.add(fn(var))

async def setup_binary_sensor(config, key, fn):
    if key in config:
        var = await binary_sensor.new_binary_sensor(config[key])
        cg.add(fn(var))

async def to_code(config):
    var = await new_ehu(config)
    await fan.register_fan(var, config)

    await setup_sensor(config, CONF_TEMPERATURE, var.set_temperature_sensor)
    await setup_sensor(config, CONF_HUMIDITY, var.set_humidity_sensor)

    await setup_switch(config, CONF_WARM_MIST, var.set_warm_mist_switch)
    await setup_switch(config, CONF_UV, var.set_uv_switch)
    await setup_switch(config, CONF_IONIZER, var.set_ionizer_switch)
    await setup_switch(config, CONF_LOCK, var.set_lock_switch)
    await setup_switch(config, CONF_SOUND, var.set_sound_switch)

    await setup_binary_sensor(config, CONF_WATER, var.set_water_binary_sensor)
