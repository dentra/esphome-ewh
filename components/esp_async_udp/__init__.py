import esphome.config_validation as cv

CODEOWNERS = ["@dentra"]

AUTO_LOAD = ["socket"]

CONFIG_SCHEMA = cv.All(
    cv.Schema({}),
    cv.only_on(["esp32", "esp8266"]),
)
