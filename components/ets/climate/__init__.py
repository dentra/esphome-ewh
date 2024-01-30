import esphome.config_validation as cv
from esphome.components import climate

from .. import ETS_COMPONENT_SCHEMA, ETSComponent, ets_ns, new_ets

AUTO_LOAD = ["ets"]

ETSClimate = ets_ns.class_("ETSClimate", ETSComponent)

CONFIG_SCHEMA = climate.CLIMATE_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(ETSClimate),
    }
).extend(ETS_COMPONENT_SCHEMA)


async def to_code(config):
    var = await new_ets(config)
    await climate.register_climate(var, config)
