import esphome.config_validation as cv
from esphome.components import climate

from .. import BWH_COMPONENT_SCHEMA, BWHComponent, bwh_ns, new_bwh

AUTO_LOAD = ["bwh"]

BWHClimate = bwh_ns.class_("BWHClimate", BWHComponent)

CONFIG_SCHEMA = climate.CLIMATE_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(BWHClimate),
    }
).extend(BWH_COMPONENT_SCHEMA)


async def to_code(config):
    var = await new_bwh(config)
    await climate.register_climate(var, config)
