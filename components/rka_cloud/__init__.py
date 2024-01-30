import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import switch
from esphome.const import ENTITY_CATEGORY_CONFIG

from .. import vport  # pylint: disable=relative-beyond-top-level

CODEOWNERS = ["@dentra"]
AUTO_LOAD = ["esp_async_tcp", "esp_async_udp", "switch"]
DEPENDENCIES = ["vport", "rka_api"]

CONF_MAC = "mac"
CONF_UID = "uid"
CONF_HOST = "host"
CONF_PORT = "port"
CONF_CLOUD = "cloud"
CONF_PAIR = "pair"

rka_cloud_ns = cg.esphome_ns.namespace("rka_cloud")
RKACloud = rka_cloud_ns.class_("RKACloud", switch.Switch, cg.Component)
RKACloudPair = rka_cloud_ns.class_("RKACloudPair", switch.Switch, cg.Component)


def cloud_uid(value) -> list:
    "transform cloud uid to cloud key array"
    value = f"{int(str(cv.uint32_t(value))):08d}"

    parts = [value[i : i + 2] for i in range(0, len(value), 2)]
    if len(parts) != 4:
        raise cv.Invalid("Cloud UID must consist of 6-8 decimal digits")

    parts_int = []
    if any(len(part) != 2 for part in parts):
        raise cv.Invalid("Cloud UID must be format XX")

    for part in parts:
        try:
            parts_int.append(int(part))
        except ValueError:
            # pylint: disable=raise-missing-from
            raise cv.Invalid("Cloud UID must be 4 decimal values from 00 to 99")

    return parts_int


def cloud_mac(value) -> list:
    return cv.mac_address(value).parts


CONFIG_SCHEMA = (
    switch.switch_schema(
        RKACloud,
        entity_category=ENTITY_CATEGORY_CONFIG,
        icon="mdi:cloud-outline",
        block_inverted=True,
    )
    .extend(
        {
            cv.Optional(CONF_MAC): cloud_mac,
            cv.Optional(CONF_UID): cloud_uid,
            cv.Optional(CONF_HOST, default="dongle.rusklimat.ru"): cv.domain,
            cv.Optional(CONF_PORT, default=10001): cv.uint16_t,
            cv.Optional(CONF_PAIR): switch.switch_schema(
                RKACloudPair,
                entity_category=ENTITY_CATEGORY_CONFIG,
                icon="mdi:cloud-search-outline",
                block_inverted=True,
            ),
        }
    )
    .extend(vport.VPORT_CLIENT_SCHEMA)
    .extend(cv.COMPONENT_SCHEMA)
)


def as_byte_array(parts: list, as_hex: bool = True):
    return cg.RawExpression(
        f'(const uint8_t[{len(parts)}]){{{",".join(("0x%0.2X" % x if as_hex else str(x)) for x in parts)}}}'
    )


async def to_code(config):
    prt = await vport.vport_get_var(config)
    var = await switch.new_switch(config, prt)
    await cg.register_component(var, config)

    if CONF_MAC in config:
        cg.add(var.set_mac(as_byte_array(config[CONF_MAC])))
    if CONF_UID in config:
        cg.add(var.set_uid(as_byte_array(config[CONF_UID], False)))

    cg.add(var.set_host(config[CONF_HOST]))
    cg.add(var.set_port(config[CONF_PORT]))

    # enable ota subscription
    cg.add_define("USE_OTA_STATE_CALLBACK")

    if CONF_PAIR in config:
        conf = config[CONF_PAIR]
        sens = await switch.new_switch(conf, var)
        await cg.register_component(sens, conf)
        cg.add(var.set_cloud_pair(sens))
