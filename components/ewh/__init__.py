import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation
from esphome.components import switch
from esphome.const import CONF_ICON, CONF_ID, CONF_TIME_ID, ENTITY_CATEGORY_CONFIG

from .. import rka_api  # pylint: disable=relative-beyond-top-level

CODEOWNERS = ["@dentra"]
AUTO_LOAD = ["rka_api", "switch"]

CONF_BST = "bst"
CONF_EWH_ID = "ewh_id"

# CONF_IDLE_TEMP_DROP = "idle_temp_drop"

ICON_WATER_BOILER = "mdi:water-boiler"
ICON_CLOCK = "mdi:clock"

ewh_ns = cg.esphome_ns.namespace("ewh")

EWHApi = ewh_ns.class_("EWHApi", cg.Component)
EWHComponent = ewh_ns.class_("EWHComponent", cg.Component)
BSTSwitch = EWHComponent.class_("BSTSwitch", switch.Switch)

EWHStateRef = ewh_ns.struct("ewh_state_t").operator("const").operator("ref")
EWHUpdateTrigger = ewh_ns.class_(
    "EWHUpdateTrigger", automation.Trigger.template(EWHStateRef)
)

CONFIG_SCHEMA = rka_api.api_schema(EWHApi, trigger_class=EWHUpdateTrigger).extend(
    {
        # cv.GenerateID(CONF_TIME_ID): cv.use_id(time.RealTimeClock),
    }
)

EWH_COMPONENT_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_EWH_ID): cv.use_id(EWHApi),
        cv.Optional(CONF_ICON, default=ICON_WATER_BOILER): cv.icon,
        cv.Optional(CONF_BST): switch.switch_schema(
            BSTSwitch, entity_category=ENTITY_CATEGORY_CONFIG, block_inverted=True
        ),
        # cv.Optional(CONF_IDLE_TEMP_DROP, default=5): cv.uint8_t,
    }
).extend(cv.COMPONENT_SCHEMA)


async def new_ewh(config):
    api = await cg.get_variable(config[CONF_EWH_ID])
    var = cg.new_Pvariable(config[CONF_ID], api)
    await cg.register_component(var, config)

    if CONF_BST in config:
        conf = config[CONF_BST]
        sens = cg.new_Pvariable(conf[CONF_ID], api)
        await switch.register_switch(sens, conf)
        cg.add(var.set_bst(sens))

    # cg.add(var.set_idle_temp_drop(config[CONF_IDLE_TEMP_DROP]))

    return var


async def to_code(config):
    var = await rka_api.new_api(config, EWHStateRef)
    if CONF_TIME_ID in config:
        time_ = await cg.get_variable(config[CONF_TIME_ID])
        cg.add(var.set_time_id(time_))
