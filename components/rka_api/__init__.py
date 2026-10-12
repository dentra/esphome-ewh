import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation
from esphome.components import sensor
from esphome.const import (
    CONF_ID,
    CONF_ON_STATE,
    CONF_POWER,
    CONF_TRIGGER_ID,
    DEVICE_CLASS_POWER,
    STATE_CLASS_MEASUREMENT,
    UNIT_WATT,
)

from .. import vport  # pylint: disable=relative-beyond-top-level

CODEOWNERS = ["@dentra"]
DEPENDENCIES = ["vport"]

rka_ns = cg.esphome_ns.namespace("rka_api")


def obj_const_ref(class_: cg.MockObjClass):
    return class_.operator("const").operator("ref")


def update_trigger(
    apiClass: cg.MockObjClass,
    stateStruct: cg.MockObjClass,
    listenerClass: cg.MockObjClass,
):
    return rka_ns.class_(
        "RKAUpdateTrigger", automation.Trigger.template(apiClass)
    ).template(apiClass, stateStruct, listenerClass)


def api_schema(api_class, trigger_class=None) -> cv.Schema:
    schema = cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(api_class),
        }
    ).extend(vport.VPORT_CLIENT_SCHEMA)
    if trigger_class is not None:
        schema = schema.extend(
            {
                cv.Optional(CONF_ON_STATE): automation.validate_automation(
                    {cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(trigger_class)}
                )
            }
        )
    # FIXME do we need COMPONENT_SCHEMA here?
    schema = schema.extend(cv.COMPONENT_SCHEMA)
    return schema


async def new_api(config, state_class: None):
    prt = await vport.vport_get_var(config)
    var = cg.new_Pvariable(config[CONF_ID], prt)
    # await cg.register_component(var, config)
    if state_class is not None:
        state_ref_class = obj_const_ref(state_class)
        for conf in config.get(CONF_ON_STATE, []):
            trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
            # trigger registers itself as listener in constructor
            await automation.build_automation(
                trigger, [(state_ref_class, "state")], conf
            )
    return var


CONF_IDLE_TEMP_DROP = "idle_temp_drop"

# water heater heating estimation options
HEATING_SCHEMA = cv.Schema(
    {
        # temperature drop after reaching target until heating element turns on again
        cv.Optional(CONF_IDLE_TEMP_DROP, default=5): cv.int_range(min=1, max=30),
        # estimated power consumption
        cv.Optional(CONF_POWER): sensor.sensor_schema(
            unit_of_measurement=UNIT_WATT,
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_POWER,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
    }
)


async def setup_heating(var, config):
    cg.add(var.set_idle_temp_drop(config[CONF_IDLE_TEMP_DROP]))
    if CONF_POWER in config:
        sens = await sensor.new_sensor(config[CONF_POWER])
        cg.add(var.set_power_sensor(sens))
