#!/bin/bash

if [ $USER != "vscode" ]; then
  WORKSPACE_DIR=$(realpath $(dirname $BASH_SOURCE)/..)
fi

export BUILD_DIR="$WORKSPACE_DIR/.build"

ESPHOME_LIB_DIR="$BUILD_DIR/esphome/include"

DEFS=(
  USE_VPORT_UART
)

LIB_DIR=$WORKSPACE_DIR/lib
LIB_ETL_DIR=$LIB_DIR/etl
LIB_COMPO_DIR=$LIB_DIR/esphome-components
LIB_COMPO_VPORT_DIR="$LIB_COMPO_DIR/esphome/components/vport"

SRCS=(
  "$LIB_COMPO_VPORT_DIR/*.cpp"
  # "$LIB_COMPO_DIR/esphome/components/nvs/*.cpp"
)

INCS=(
  "$LIB_COMPO_DIR"
  "$LIB_ETL_DIR/include"
)

. $(dirname $0)/_cloak/runner.sh

exit 0

# esphome_components=".esphome/include/esphome-components/"

# SRCS=(
#     components/ets/*.cpp
#     components/ets/vport/*.cpp
#     components/rka_api/*.cpp
#     components/rka_cloud/*.cpp

#     $esphome_components/esphome/components/vport/*.cpp
# )

# INCS=(
#   "$esphome_components"
#   ".esphome/libdeps/esp8266-arduino/Embedded Template Library/include"
# )

# DEFS=(
#   USE_VPORT_UART
# )

# LIBS=()

# FLAGS=()

# . tests/_cloak/runner.sh
#!/bin/bash

ESPHOME_LIB_DIR="$WORKSPACE_DIR/.esphome/include"
ARDUINO_LIB_DIR="$WORKSPACE_DIR/.esphome/libdeps/esp8266-arduino/"

DEFS=(
  USE_VPORT_UART
)

SRCS=(
  "$ESPHOME_LIB_DIR/esphome-components/esphome/components/vport/*.cpp"
)

INCS=(
  "$ESPHOME_LIB_DIR/esphome-components"
  "$ARDUINO_LIB_DIR/Embedded Template Library/include"
)

SRCS_FILTER=".*/(esp32_usb_dis|logger|wifi|_fastled_base|led_strip_spi)/.+\\.cpp$"
# SRCS_FILTER=".*/led_strip_spi/.+\\cpp$"

.  $(dirname $0)/_cloak/runner.sh
