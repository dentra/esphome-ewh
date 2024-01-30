import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation
from esphome.const import CONF_ICON, CONF_ID, CONF_TIME_ID

from .. import rka_api  # pylint: disable=relative-beyond-top-level

CODEOWNERS = ["@dentra"]
AUTO_LOAD = ["rka_api", "switch"]

CONF_BWH_ID = "bwh_id"

ICON_WATER_BOILER = "mdi:water-boiler"
ICON_CLOCK = "mdi:clock"

bwh_ns = cg.esphome_ns.namespace("bwh")

BWHApi = bwh_ns.class_("BWHApi", cg.Component)
BWHComponent = bwh_ns.class_("EWHComponent", cg.Component)

BWHStateRef = bwh_ns.struct("ewh_state_t").operator("const").operator("ref")
BWHUpdateTrigger = bwh_ns.class_(
    "EWHUpdateTrigger", automation.Trigger.template(BWHStateRef)
)

CONFIG_SCHEMA = rka_api.api_schema(BWHApi, trigger_class=BWHUpdateTrigger)

BWH_COMPONENT_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_BWH_ID): cv.use_id(BWHApi),
        cv.Optional(CONF_ICON, default=ICON_WATER_BOILER): cv.icon,
    }
).extend(cv.COMPONENT_SCHEMA)


async def new_bwh(config):
    api = await cg.get_variable(config[CONF_BWH_ID])
    var = cg.new_Pvariable(config[CONF_ID], api)
    await cg.register_component(var, config)

    return var


async def to_code(config):
    var = await rka_api.new_api(config, BWHStateRef)
    if CONF_TIME_ID in config:
        time_ = await cg.get_variable(config[CONF_TIME_ID])
        cg.add(var.set_time_id(time_))
