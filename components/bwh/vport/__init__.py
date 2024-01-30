import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart, vport
from esphome.const import CONF_ID
from ... import vport  # pylint: disable=relative-beyond-top-level

AUTO_LOAD = ["vport"]

bwh_ns = cg.esphome_ns.namespace("bwh")
BWHVPort = bwh_ns.class_("BWHVPort", cg.Component, vport.VPort)
BWHUartIO = bwh_ns.class_("BWHUartIO")

CONFIG_SCHEMA = vport.vport_uart_schema(BWHVPort, BWHUartIO)


async def to_code(config):
    var = await vport.setup_vport_uart(config)
