import esphome.codegen as cg

from ... import vport  # pylint: disable=relative-beyond-top-level

AUTO_LOAD = ["vport"]

ewh_ns = cg.esphome_ns.namespace("ewh")
EWHVPort = ewh_ns.class_("EWHVPort", cg.Component, vport.VPort)
EWHUartIO = ewh_ns.class_("EWHUartIO")

CONFIG_SCHEMA = vport.vport_uart_schema(EWHVPort, EWHUartIO)


async def to_code(config):
    await vport.setup_vport_uart(config)
