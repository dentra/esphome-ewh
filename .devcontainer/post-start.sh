#!/bin/bash

set -e

if [ ! -L esphome ]; then
    ln -sv $(python3 -c "import esphome as _; print(_.__path__[0])") esphome
fi

find . -type d -name .esphome -exec sudo chown -R vscode:vscode {} \;
find . -type d -name .pio -exec sudo chown -R vscode:vscode {} \;