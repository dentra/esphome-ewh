import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation
from esphome.components import sensor
from esphome.const import (
    CONF_ICON,
    CONF_ID,
    DEVICE_CLASS_TEMPERATURE,
    STATE_CLASS_MEASUREMENT,
    UNIT_CELSIUS,
)

from .. import rka_api  # pylint: disable=relative-beyond-top-level

CODEOWNERS = ["@dentra"]
AUTO_LOAD = ["rka_api", "sensor"]

CONF_ETS_ID = "ets_id"
CONF_FLOOR_TEMP = "floor_temperature"

ICON_THERMOSTAT = "mdi:thermostat"
ICON_CLOCK = "mdi:clock"

ets_ns = cg.esphome_ns.namespace("ets")

ETSApi = ets_ns.class_("ETSApi", cg.Component)
ETSComponent = ets_ns.class_("ETSComponent", cg.PollingComponent)
ETSState = ets_ns.struct("ets_state_t")
ETSUpdateTrigger = ets_ns.class_(
    "ETSUpdateTrigger", automation.Trigger.template(rka_api.obj_const_ref(ETSState))
)


CONFIG_SCHEMA = rka_api.api_schema(ETSApi, trigger_class=ETSUpdateTrigger)

ETS_COMPONENT_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_ETS_ID): cv.use_id(ETSApi),
        cv.Optional(CONF_ICON, default=ICON_THERMOSTAT): cv.icon,
        cv.Optional(CONF_FLOOR_TEMP): sensor.sensor_schema(
            unit_of_measurement=UNIT_CELSIUS,
            icon="mdi:heated-floor",
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_TEMPERATURE,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
    }
).extend(cv.polling_component_schema("30s"))


async def new_ets(config):
    api = await cg.get_variable(config[CONF_ETS_ID])
    var = cg.new_Pvariable(config[CONF_ID], api)
    await cg.register_component(var, config)

    if CONF_FLOOR_TEMP in config:
        sens = await sensor.new_sensor(config[CONF_FLOOR_TEMP])
        cg.add(var.set_floor_temp(sens))

    return var


async def to_code(config):
    var = await rka_api.new_api(config, ETSState)
