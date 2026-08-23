#!/bin/bash

set -e

command -v openocd >/dev/null 2>&1 || { echo "ERROR: openocd not found. Install: sudo apt install openocd"; exit 1; }

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
BIN="${BUILD_DIR}/stm32.bin"

if [ ! -f "${BIN}" ]; then
    echo "ERROR: ${BIN} not found. Run ./build.sh first."
    exit 1
fi

# 烧录地址 STM32F1内部Flash起始地址
FLASH_ADDR="0x08000000"

# 用stlink接口+stm32f1x自动识别(适配各密度型号)
# 部分STM32F1批次IDCODE版本位不同(0x1ba vs 0x2ba), 在加载target配置前覆盖CPUTAPID
# 如需指定板子配置可换成: -f board/stm32f103c8_blue_pill.cfg
openocd -c "set CPUTAPID 0x2ba01477" \
        -f interface/stlink.cfg \
        -f target/stm32f1x.cfg \
        -c "program ${BIN} ${FLASH_ADDR} verify reset exit"

echo "=== Flashed ${BIN} to ${FLASH_ADDR} ==="
