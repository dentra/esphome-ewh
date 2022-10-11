import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation
from esphome.components import switch, time, uart
from esphome.const import (
    CONF_ICON,
    CONF_ID,
    CONF_ON_STATE,
    CONF_TIME_ID,
    CONF_TRIGGER_ID,
    ENTITY_CATEGORY_CONFIG,
)

CODEOWNERS = ["@dentra"]
AUTO_LOAD = ["switch"]

CONF_BST = "bst"
CONF_EWH_ID = "ewh_id"

# CONF_IDLE_TEMP_DROP = "idle_temp_drop"

ICON_WATER_BOILER = "mdi:water-boiler"
ICON_CLOCK = "mdi:clock"


ewh_ns = cg.esphome_ns.namespace("ewh")

EWH = ewh_ns.class_("ElectroluxWaterHeater", cg.Component, uart.UARTDevice)
EWHListener = ewh_ns.class_("EWHListener")
EWHComponent = ewh_ns.class_("EWHComponent", cg.Component, EWHListener)
BSTSwitch = EWHComponent.class_("BSTSwitch", switch.Switch)

EWHStatusRef = ewh_ns.struct("ewh_status_t").operator("const").operator("ref")
EWHUpdateTrigger = ewh_ns.class_(
    "EWHUpdateTrigger", automation.Trigger.template(EWHStatusRef)
)


CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(EWH),
            cv.GenerateID(CONF_TIME_ID): cv.use_id(time.RealTimeClock),
            cv.Optional(CONF_ON_STATE): automation.validate_automation(
                {
                    cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(EWHUpdateTrigger),
                }
            ),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(uart.UART_DEVICE_SCHEMA)
)

EWH_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_EWH_ID): cv.use_id(EWH),
    }
)

EWH_COMPONENT_SCHEMA = (
    cv.Schema(
        {
            cv.Optional(CONF_ICON, default=ICON_WATER_BOILER): cv.icon,
            cv.Optional(CONF_BST): switch.switch_schema(
                BSTSwitch, entity_category=ENTITY_CATEGORY_CONFIG, block_inverted=True
            ),
            # cv.Optional(CONF_IDLE_TEMP_DROP, default=5): cv.uint8_t,
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(EWH_SCHEMA)
)


async def new_ewh(config):
    ewh_ = await cg.get_variable(config[CONF_EWH_ID])
    var = cg.new_Pvariable(config[CONF_ID], ewh_)
    await cg.register_component(var, config)

    if CONF_BST in config:
        conf = config[CONF_BST]
        sens = cg.new_Pvariable(conf[CONF_ID], ewh_)
        await switch.register_switch(sens, conf)
        cg.add(var.set_bst(sens))

    # cg.add(var.set_idle_temp_drop(config[CONF_IDLE_TEMP_DROP]))

    return var


async def to_code(config):

    urt = await cg.get_variable(config[uart.CONF_UART_ID])
    var = cg.new_Pvariable(config[CONF_ID], urt)
    await cg.register_component(var, config)
    if CONF_TIME_ID in config:
        time_ = await cg.get_variable(config[CONF_TIME_ID])
        cg.add(var.set_time_id(time_))
    for conf in config.get(CONF_ON_STATE, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [(EWHStatusRef, "state")], conf)
