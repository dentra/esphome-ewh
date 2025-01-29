import esphome.codegen as cg

from ... import vport  # pylint: disable=relative-beyond-top-level

AUTO_LOAD = ["vport"]

ehu_ns = cg.esphome_ns.namespace("ehu")
EHUVPort = ehu_ns.class_("EHUVPort", cg.Component, vport.VPort)
EHUUartIO = ehu_ns.class_("EHUUartIO")

CONFIG_SCHEMA = vport.vport_uart_schema(EHUVPort, EHUUartIO)


async def to_code(config):
    await vport.setup_vport_uart(config)
