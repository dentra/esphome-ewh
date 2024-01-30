import esphome.config_validation as cv
from esphome.components import climate
from .. import EWH_COMPONENT_SCHEMA, EWHComponent, ewh_ns, new_ewh

AUTO_LOAD = ["ewh"]

EWHClimate = ewh_ns.class_("EWHClimate", EWHComponent)

CONFIG_SCHEMA = climate.CLIMATE_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(EWHClimate),
    }
).extend(EWH_COMPONENT_SCHEMA)


async def to_code(config):
    var = await new_ewh(config)
    await climate.register_climate(var, config)
