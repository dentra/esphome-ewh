from esphome.components import switch

from .. import EHU_COMPONENT_SCHEMA, EHUComponent, ehu_ns, new_ehu

AUTO_LOAD = ["ehu"]

EHUPowerSwitch = ehu_ns.class_("EHUPowerSwitch", EHUComponent)

CONFIG_SCHEMA = switch.switch_schema(
    EHUPowerSwitch, icon="mdi:power-standby", block_inverted=True
).extend(EHU_COMPONENT_SCHEMA)

async def to_code(config):
    var = await new_ehu(config)
    await switch.register_switch(var, config)


