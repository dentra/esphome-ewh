import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation
from esphome.const import CONF_ID, CONF_ON_STATE, CONF_TRIGGER_ID
from .. import vport  # pylint: disable=relative-beyond-top-level


CODEOWNERS = ["@dentra"]
DEPENDENCIES = ["vport"]
AUTO_LOAD = ["etl"]


def api_schema(
    api_class, default_update_interval=None, trigger_class=None
) -> cv.Schema:
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
    if default_update_interval is None:
        schema = schema.extend(cv.COMPONENT_SCHEMA)
    else:
        schema = schema.extend(cv.polling_component_schema(default_update_interval))
    return schema


async def new_api(config, state_ref_class: None):
    prt = await vport.vport_get_var(config)
    var = cg.new_Pvariable(config[CONF_ID], prt)
    # await cg.register_component(var, config)
    if state_ref_class is not None:
        for conf in config.get(CONF_ON_STATE, []):
            trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
            await automation.build_automation(
                trigger, [(state_ref_class, "state")], conf
            )
            cg.add(var.add_listener(trigger))
    return var
