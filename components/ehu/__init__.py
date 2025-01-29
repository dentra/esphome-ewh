import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation
from esphome.const import CONF_ICON, CONF_ID, CONF_TIME_ID

from .. import rka_api  # pylint: disable=relative-beyond-top-level

CODEOWNERS = ["@dentra"]
AUTO_LOAD = ["rka_api", "switch"]

CONF_EHU_ID = "ehu_id"

ICON_HUMIDIFIER = "mdi:air-humidifier"
ICON_CLOCK = "mdi:clock"

ehu_ns = cg.esphome_ns.namespace("ehu")

EHUApi = ehu_ns.class_("EHUApi", cg.Component)
EHUComponent = ehu_ns.class_("EHUComponent", cg.Component)

EHUStateRef = ehu_ns.struct("ehu_state_t").operator("const").operator("ref")
EHUUpdateTrigger = ehu_ns.class_(
    "EHUUpdateTrigger", automation.Trigger.template(EHUStateRef)
)

CONFIG_SCHEMA = rka_api.api_schema(EHUApi, trigger_class=EHUUpdateTrigger)

EHU_COMPONENT_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_EHU_ID): cv.use_id(EHUApi),
        cv.Optional(CONF_ICON, default=ICON_HUMIDIFIER): cv.icon,
    }
).extend(cv.polling_component_schema("15s"))


async def new_ehu(config):
    api = await cg.get_variable(config[CONF_EHU_ID])
    var = cg.new_Pvariable(config[CONF_ID], api)
    await cg.register_component(var, config)

    return var


async def to_code(config):
    var = await rka_api.new_api(config, EHUStateRef)
    if CONF_TIME_ID in config:
        time_ = await cg.get_variable(config[CONF_TIME_ID])
        cg.add(var.set_time_id(time_))
