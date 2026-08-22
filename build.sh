#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"

# cmake检查
command -v cmake >/dev/null 2>&1 || { echo "ERROR: cmake not found. Install: sudo apt install cmake"; exit 1; }
# 交叉编译器检查
command -v arm-none-eabi-gcc >/dev/null 2>&1 || { echo "ERROR: arm-none-eabi-gcc not found. Install: sudo apt install gcc-arm-none-eabi"; exit 1; }

# st的mcu package
ST_MCU_PATH="/home/dingrui/MyApp/st/HAL/STM32Cube_FW_F1_V1.8.0"
# cmake编译的时候要用这个参数 用-D传给cmake
export ST_MCU_PATH

# hal文件
if [ ! "$(ls -A "${SCRIPT_DIR}/lib" 2>/dev/null)" ]; then
    echo "lib/ is empty, copying HAL driver from ST package..."
    mkdir -p "${SCRIPT_DIR}/lib/Inc" "${SCRIPT_DIR}/lib/Src"
    cp -r "${ST_MCU_PATH}/Drivers/STM32F1xx_HAL_Driver/Inc/." "${SCRIPT_DIR}/lib/Inc/"
    cp -r "${ST_MCU_PATH}/Drivers/STM32F1xx_HAL_Driver/Src/." "${SCRIPT_DIR}/lib/Src/"
fi

# hal配置头文件(模板拷贝并裁剪,只用其中一部分模块时手动精简)
if [ ! -f "${SCRIPT_DIR}/lib/Inc/stm32f1xx_hal_conf.h" ]; then
    echo "stm32f1xx_hal_conf.h not found, copying from template..."
    cp "${SCRIPT_DIR}/lib/Inc/stm32f1xx_hal_conf_template.h" "${SCRIPT_DIR}/lib/Inc/stm32f1xx_hal_conf.h"
fi

if [ ! "$(ls -A "${SCRIPT_DIR}/cmsis" 2>/dev/null)" ]; then
    echo "cmsis/ is empty, copying CMSIS from ST package..."
    mkdir -p "${SCRIPT_DIR}/cmsis/Include" "${SCRIPT_DIR}/cmsis/Src"
    cp -r "${ST_MCU_PATH}/Drivers/CMSIS/Include/." "${SCRIPT_DIR}/cmsis/Include/"
    cp -r "${ST_MCU_PATH}/Drivers/CMSIS/Device/ST/STM32F1xx/Include/." "${SCRIPT_DIR}/cmsis/Include/"
    cp "${ST_MCU_PATH}/Drivers/CMSIS/Device/ST/STM32F1xx/Source/Templates/system_stm32f1xx.c" "${SCRIPT_DIR}/cmsis/Src/"
    # 注意:gcc编译器用Templates/gcc下的启动文件(arm/是keil版的)
    cp "${ST_MCU_PATH}/Drivers/CMSIS/Device/ST/STM32F1xx/Source/Templates/gcc/startup_stm32f103xb.s" "${SCRIPT_DIR}/cmsis/Src/"
fi

# 链接脚本(和上面的启动文件/设备宏对应)
if [ ! -f "${SCRIPT_DIR}/STM32F103XB_FLASH.ld" ]; then
    echo "linker script not found, copying from ST package..."
    cp "${ST_MCU_PATH}/Drivers/CMSIS/Device/ST/STM32F1xx/Source/Templates/gcc/linker/STM32F103XB_FLASH.ld" "${SCRIPT_DIR}/"
fi

# build
echo "Configuring build..."
mkdir -p "${BUILD_DIR}"
cmake -B "${BUILD_DIR}" -S . -DST_MCU_PATH="${ST_MCU_PATH}"

echo "Building..."
cmake --build "${BUILD_DIR}" -j$(nproc)

echo ""
echo "=== Build complete ==="
echo "Artifacts:"
ls -la "${BUILD_DIR}/stm32" "${BUILD_DIR}/stm32.hex" "${BUILD_DIR}/stm32.bin"
echo ""
echo "Flash with ST-Link:"
echo "  st-flash write ${BUILD_DIR}/stm32.bin 0x08000000"
