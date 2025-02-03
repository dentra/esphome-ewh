import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import fan, sensor
from esphome.const import (
    CONF_HUMIDITY,
    CONF_NAME,
    CONF_TEMPERATURE,
    DEVICE_CLASS_HUMIDITY,
    DEVICE_CLASS_TEMPERATURE,
    STATE_CLASS_MEASUREMENT,
    UNIT_CELSIUS,
    UNIT_PERCENT,
)

from .. import EHU_COMPONENT_SCHEMA, EHUComponent, ehu_ns, new_ehu

AUTO_LOAD = ["ehu", "fan", "sensor"]

EHUFan = ehu_ns.class_("EHUFan", EHUComponent)

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

    }
).extend(EHU_COMPONENT_SCHEMA)

async def setup_sensor(config, key, fn):
    if conf := config.get(key):
        sens = await sensor.new_sensor(conf)
        cg.add(fn(sens))

async def to_code(config):
    var = await new_ehu(config)
    await fan.register_fan(var, config)

    await setup_sensor(config, CONF_TEMPERATURE, var.set_temperature_sensor)
    await setup_sensor(config, CONF_HUMIDITY, var.set_humidity_sensor)

