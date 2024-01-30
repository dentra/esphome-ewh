import esphome.codegen as cg

from ... import vport  # pylint: disable=relative-beyond-top-level

AUTO_LOAD = ["vport"]

ets_ns = cg.esphome_ns.namespace("ets")
ETSVPort = ets_ns.class_("ETSVPort", cg.Component, vport.VPort)
ETSUartIO = ets_ns.class_("ETSUartIO")

CONFIG_SCHEMA = vport.vport_uart_schema(ETSVPort, ETSUartIO)


async def to_code(config):
    await vport.setup_vport_uart(config)
