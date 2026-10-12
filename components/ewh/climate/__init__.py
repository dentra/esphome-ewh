from esphome.components import climate

from ... import rka_api  # pylint: disable=relative-beyond-top-level
from .. import EWH_COMPONENT_SCHEMA, EWHComponent, ewh_ns, new_ewh

AUTO_LOAD = ["ewh", "sensor"]

EWHClimate = ewh_ns.class_("EWHClimate", EWHComponent)

CONFIG_SCHEMA = (
    climate.climate_schema(EWHClimate)
    .extend(rka_api.HEATING_SCHEMA)
    .extend(EWH_COMPONENT_SCHEMA)
)


async def to_code(config):
    var = await new_ewh(config)
    await climate.register_climate(var, config)
    await rka_api.setup_heating(var, config)
