import esphome.config_validation as cv
import esphome.codegen as cg
from esphome.core import CORE
from .. import ewh
from esphome.const import CONF_ID

CODEOWNERS = ["@dentra"]
AUTO_LOAD = ["async_tcp", "text_sensor", "ewh"]

CONF_MAC = "mac"
CONF_UID = "uid"
CONF_HOST = "host"
CONF_PORT = "port"

EWHCloud = ewh.ewh_ns.class_("EWHCloud", cg.Component, ewh.EWHListener)


def cloud_uid(value) -> list:
    "transform cloud uid to cloud key array"
    value = f"{int(value):06d}"

    parts = [value[i : i + 2] for i in range(0, len(value), 2)]
    if len(parts) != 3:
        raise cv.Invalid("Cloud UID must consist of 6 decimal digits")

    parts_int = []
    if any(len(part) != 2 for part in parts):
        raise cv.Invalid("Cloud UID must be format XX")

    for part in parts:
        try:
            parts_int.append(int(part))
        except ValueError:
            # pylint: disable=raise-missing-from
            raise cv.Invalid("Cloud UID must be 3 decimal values from 00 to 99")

    return parts_int


def cloud_mac(value) -> list:
    return cv.mac_address(value).parts


CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(EWHCloud),
            cv.Optional(CONF_MAC): cloud_mac,
            cv.Optional(CONF_UID): cloud_uid,
            cv.Optional(CONF_HOST, default="dongle.rusklimat.ru"): cv.domain,
            cv.Optional(CONF_PORT, default=10001): cv.uint16_t,
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(ewh.EWH_SCHEMA)
)


def as_byte_array(parts: list, as_hex: bool = True):
    return cg.RawExpression(
        f'(const uint8_t[{len(parts)}]){{{",".join(("0x%0.2X" % x if as_hex else str(x)) for x in parts)}}}'
    )


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    ewh_ = await cg.get_variable(config[ewh.CONF_EWH_ID])
    cg.add(ewh_.add_listener(var))

    if CONF_MAC in config:
        cg.add(var.set_mac(as_byte_array(config[CONF_MAC])))
    if CONF_UID in config:
        cg.add(var.set_key(as_byte_array(config[CONF_UID], False)))
    cg.add(var.set_host(config[CONF_HOST]))
    cg.add(var.set_port(config[CONF_PORT]))

    if CORE.is_esp8266:
        cg.add_library("", "", "https://github.com/me-no-dev/ESPAsyncUDP.git")
