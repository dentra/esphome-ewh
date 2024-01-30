import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.core import CORE, coroutine_with_priority

CODEOWNERS = ["@dentra"]


def AUTO_LOAD():
    if CORE.using_arduino:
        return ["etl", "async_tcp"]
    return ["etl"]


CONFIG_SCHEMA = cv.All(
    cv.Schema({}),
    cv.only_on(["esp32", "esp8266"]),
)


@coroutine_with_priority(200.0)
async def to_code(config):
    # if CORE.using_arduino:
    #     cg.add_build_flag("-DLWIP_SOCKET=1")
    #     cg.add_build_flag("-DLWIP_COMPAT_SOCKETS=1")
    pass
