import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.core import CORE, coroutine_with_priority

CODEOWNERS = ["@dentra"]

AUTO_LOAD = ["etl"]

CONFIG_SCHEMA = cv.All(
    cv.Schema({}),
    cv.only_on(["esp32", "esp8266"]),
)


@coroutine_with_priority(200.0)
async def to_code(config):
    if CORE.using_arduino:
        if CORE.is_esp8266:
            cg.add_library("", "", "https://github.com/me-no-dev/ESPAsyncUDP.git")
            cg.add_build_flag("-DLWIP_SOCKET=1")
            cg.add_build_flag("-DLWIP_COMPAT_SOCKETS=0")
            cg.add_build_flag("-DLWIP_TIMEVAL_PRIVATE=0")

        # if CORE.is_esp32:
        # cg.add_library("AsyncUDP", None)
        # cg.add_build_flag("-DLWIP_SOCKET=1")
        # cg.add_build_flag("-DLWIP_COMPAT_SOCKETS=1")
        # cg.add_build_flag("-DLWIP_POSIX_SOCKETS_IO_NAMES")

    # - -DPSTR_ALIGN=1
    # - -DLWIP_SOCKET=1
    # - -DLWIP_COMPAT_SOCKETS=0
    # - -DESP_SOCKET=1
    # - -DLWIP_TIMEVAL_PRIVATE=0
    # # - -DLWIP_POSIX_SOCKETS_IO_NAMES=1
    # - -DUSE_RKA_SOCKETS
