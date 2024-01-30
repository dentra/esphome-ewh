import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart, vport
from esphome.const import CONF_ID
from ... import vport  # pylint: disable=relative-beyond-top-level

AUTO_LOAD = ["vport"]

ewh_ns = cg.esphome_ns.namespace("ewh")
EWHVPort = ewh_ns.class_("EWHVPort", cg.Component, vport.VPort)
EWHUartIO = ewh_ns.class_("EWHUartIO")

CONFIG_SCHEMA = vport.vport_uart_schema(EWHVPort, EWHUartIO)


async def to_code(config):
    var = await vport.setup_vport_uart(config)
