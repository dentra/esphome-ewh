from esphome.components import climate

from ... import rka_api  # pylint: disable=relative-beyond-top-level
from .. import BWH_COMPONENT_SCHEMA, BWHComponent, bwh_ns, new_bwh

AUTO_LOAD = ["bwh", "sensor"]

BWHClimate = bwh_ns.class_("BWHClimate", BWHComponent)

CONFIG_SCHEMA = (
    climate.climate_schema(BWHClimate)
    .extend(rka_api.HEATING_SCHEMA)
    .extend(BWH_COMPONENT_SCHEMA)
)


async def to_code(config):
    var = await new_bwh(config)
    await climate.register_climate(var, config)
    await rka_api.setup_heating(var, config)
