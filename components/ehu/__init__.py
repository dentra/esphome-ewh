import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation
from esphome.components import binary_sensor, switch
from esphome.const import (
    CONF_ICON,
    CONF_ID,
    CONF_NAME,
    CONF_TIME_ID,
    DEVICE_CLASS_MOISTURE,
    ENTITY_CATEGORY_CONFIG,
)

from .. import rka_api  # pylint: disable=relative-beyond-top-level

CODEOWNERS = ["@dentra"]
AUTO_LOAD = ["rka_api", "switch", "binary_sensor"]

CONF_EHU_ID = "ehu_id"

ICON_HUMIDIFIER = "mdi:air-humidifier"
ICON_CLOCK = "mdi:clock"

CONF_WARM_MIST="warm_mist"
CONF_UV="uv"
CONF_IONIZER="ionizer"
CONF_LOCK="lock"
CONF_SOUND="sound"
CONF_WATER="water"


ehu_ns = cg.esphome_ns.namespace("ehu")

EHUApi = ehu_ns.class_("EHUApi", cg.Component)
EHUComponent = ehu_ns.class_("EHUComponent", cg.Component)

EHUStateRef = ehu_ns.struct("ehu_state_t").operator("const").operator("ref")
EHUUpdateTrigger = ehu_ns.class_(
    "EHUUpdateTrigger", automation.Trigger.template(EHUStateRef)
)

EHUSwitch= ehu_ns.class_("EHUSwitch", switch.Switch, cg.Component)
EHUWarmMistSwitch= ehu_ns.class_("EHUWarmMistSwitch", switch.Switch, cg.Component)
EHUUVSwitch=ehu_ns.class_("EHUUVSwitch", switch.Switch, cg.Component)

EHUPacketType=ehu_ns.enum("ehu_packet_type_t", is_class=True)

CONFIG_SCHEMA = rka_api.api_schema(EHUApi, trigger_class=EHUUpdateTrigger)

EHU_COMPONENT_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_EHU_ID): cv.use_id(EHUApi),
        cv.Optional(CONF_ICON, default=ICON_HUMIDIFIER): cv.icon,

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
).extend(cv.polling_component_schema("15s"))

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

async def new_ehu(config):
    api = await cg.get_variable(config[CONF_EHU_ID])
    var = cg.new_Pvariable(config[CONF_ID], api)
    await cg.register_component(var, config)

    await setup_switch(config, CONF_WARM_MIST, var.set_warm_mist_switch)
    await setup_switch(config, CONF_UV, var.set_uv_switch)
    await setup_switch(config, CONF_IONIZER, var.set_ionizer_switch)
    await setup_switch(config, CONF_LOCK, var.set_lock_switch)
    await setup_switch(config, CONF_SOUND, var.set_sound_switch)

    await setup_binary_sensor(config, CONF_WATER, var.set_water_binary_sensor)

    return var


async def to_code(config):
    var = await rka_api.new_api(config, EHUStateRef)
    if CONF_TIME_ID in config:
        time_ = await cg.get_variable(config[CONF_TIME_ID])
        cg.add(var.set_time_id(time_))
