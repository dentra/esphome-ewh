import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import fan, sensor, switch
from esphome.const import (
    CONF_HUMIDITY,
    CONF_NAME,
    CONF_TEMPERATURE,
    DEVICE_CLASS_HUMIDITY,
    DEVICE_CLASS_TEMPERATURE,
    ENTITY_CATEGORY_CONFIG,
    STATE_CLASS_MEASUREMENT,
    UNIT_CELSIUS,
    UNIT_PERCENT,
)

from .. import CONF_EHU_ID, EHU_COMPONENT_SCHEMA, EHUComponent, ehu_ns, new_ehu

AUTO_LOAD = ["ehu"]

CONF_WARM_MIST="warm_mist"
CONF_UV="uv"
CONF_IONIZER="ionizer"
CONF_LOCK="lock"
CONF_SOUND="sound"

EHUFan = ehu_ns.class_("EHUFan", EHUComponent)
EHUSwitch= ehu_ns.class_("EHUSwitch", switch.Switch, cg.Component)

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
                EHUSwitch,
                entity_category=ENTITY_CATEGORY_CONFIG,
                block_inverted=True,
            ),
            key=CONF_NAME,
        ),
        cv.Optional(CONF_UV): cv.maybe_simple_value(
            switch.switch_schema(
                EHUSwitch,
                entity_category=ENTITY_CATEGORY_CONFIG,
                block_inverted=True,
            ),
            key=CONF_NAME,
        ),
        cv.Optional(CONF_IONIZER): cv.maybe_simple_value(
            switch.switch_schema(
                EHUSwitch,
                entity_category=ENTITY_CATEGORY_CONFIG,
                block_inverted=True,
            ),
            key=CONF_NAME,
        ),
        cv.Optional(CONF_LOCK): cv.maybe_simple_value(
            switch.switch_schema(
                EHUSwitch,
                entity_category=ENTITY_CATEGORY_CONFIG,
                block_inverted=True,
            ),
            key=CONF_NAME,
        ),
        cv.Optional(CONF_SOUND): cv.maybe_simple_value(
            switch.switch_schema(
                EHUSwitch,
                entity_category=ENTITY_CATEGORY_CONFIG,
                block_inverted=True,
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
        api = await cg.get_variable(config[CONF_EHU_ID])
        var = await switch.new_switch(config[key], api)
        await cg.register_component(var, config[key])
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
