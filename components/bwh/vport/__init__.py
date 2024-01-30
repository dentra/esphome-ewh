import esphome.codegen as cg

from ... import vport  # pylint: disable=relative-beyond-top-level

AUTO_LOAD = ["vport"]

bwh_ns = cg.esphome_ns.namespace("bwh")
BWHVPort = bwh_ns.class_("BWHVPort", cg.Component, vport.VPort)
BWHUartIO = bwh_ns.class_("BWHUartIO")

CONFIG_SCHEMA = vport.vport_uart_schema(BWHVPort, BWHUartIO)


async def to_code(config):
    await vport.setup_vport_uart(config)
