#!/bin/bash

set -e

# Windows版OpenOCD路径
OPENOCD_EXE="/mnt/c/MyApp/openocd/xpack-openocd-0.12.0-7/bin/openocd.exe"
OPENOCD_SCRIPTS="C:/MyApp/openocd/xpack-openocd-0.12.0-7/share/openocd/scripts"

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
BIN="${BUILD_DIR}/stm32.bin"

if [ ! -f "${BIN}" ]; then
    echo "ERROR: ${BIN} not found. Run ./build.sh first."
    exit 1
fi

# 烧录地址 STM32F1内部Flash起始地址
FLASH_ADDR="0x08000000"

# 部分STM32F1批次IDCODE版本位不同(0x1ba vs 0x2ba), 在加载target配置前覆盖CPUTAPID
# 如需指定板子配置可换成: -f board/stm32f103c8_blue_pill.cfg

if [ -f "${OPENOCD_EXE}" ]; then
    # Windows原生OpenOCD 通过WSL互操作调用
    # 反斜杠在WSL->Windows传参时会被吞 转成正斜杠
    BIN_WIN="$(wslpath -w "${BIN}" | tr '\\' '/')"
    "${OPENOCD_EXE}" -c "set CPUTAPID 0x2ba01477" \
            -s "${OPENOCD_SCRIPTS}" \
            -f interface/stlink.cfg \
            -f target/stm32f1x.cfg \
            -c "program ${BIN_WIN} ${FLASH_ADDR} verify reset exit"
else
    # 回退到WSL内的OpenOCD(需要usbipd透传)
    command -v openocd >/dev/null 2>&1 || { echo "ERROR: ${OPENOCD_EXE} not found, and openocd not in WSL. Install: sudo apt install openocd"; exit 1; }
    openocd -c "set CPUTAPID 0x2ba01477" \
            -f interface/stlink.cfg \
            -f target/stm32f1x.cfg \
            -c "program ${BIN} ${FLASH_ADDR} verify reset exit"
fi

echo "=== Flashed ${BIN} to ${FLASH_ADDR} ==="
