#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"

# 芯片型号 用命令行参数指定 例如./build.sh STM32F103C8T6
# 也可以直接给密度档位宏 例如./build.sh STM32F103xB
MCU="${1:-stm32f103c8t6}"
MCU_UC="$(printf '%s' "${MCU}" | tr '[:lower:]' '[:upper:]')"
# 示例工程目录名 用于从ST包里拷贝中断处理文件(不同板子路径不同 可用环境变量覆盖)
BOARD_EXAMPLE="${BOARD_EXAMPLE:-STM32F103RB-Nucleo}"
# st的mcu package 可用环境变量覆盖
ST_MCU_PATH="${ST_MCU_PATH:-/home/dingrui/MyApp/st/package/STM32Cube_FW_F1_V1.8.0}"

# 按ST命名规则解析 STM32F{家族}{引脚}{Flash容量}{封装}{温度}
# 其中Flash容量位(C8T6的8)决定密度档位宏 映射关系与stm32f1xx.h一致
# 4/6映射x6 8/B映射xB C/D/E映射xE F/G映射xG F105/107映射xC
# 然后自动推导启动文件和链接脚本
# 直接传密度档位时保留原大小写(STM32F103xB的小写x必须保留 否则与stm32f1xx.h的defined()对不上 编译器会当成F105系列)
# 型号->密度档位宏(对应stm32f1xx.h里的定义)
# 命名 STM32F+家族(3位数字)+引脚(1字符)+Flash容量(1字符)+封装(1字符)+温度(1字符)
# 例如 STM32F103C8T6->Flash容量位8->STM32F103xB
mcu_to_macro() {
    local mcu="$1" family flash den
    family="${mcu:6:3}"
    flash="${mcu:10:1}"
    case "${family}" in
        100)
            case "${flash}" in C|D|E) den="E" ;; 4|6|8|B) den="B" ;; *) return 1 ;; esac ;;
        101|103)
            case "${flash}" in 4|6) den="6" ;; 8|B) den="B" ;; C|D|E) den="E" ;; F|G) den="G" ;; *) return 1 ;; esac ;;
        102)
            case "${flash}" in 4|6) den="6" ;; 8|B) den="B" ;; *) return 1 ;; esac ;;
        105|107)
            den="C" ;;
        *) return 1 ;;
    esac
    printf 'STM32F%sx%s' "${family}" "${den}"
}

# 判断传的是完整型号还是密度档位宏(如STM32F103xB)
if [[ "${MCU_UC}" =~ ^STM32F[0-9]{3}[xX][A-Z0-9]$ ]]; then
    # 密度档位宏保留原大小写 如STM32F103xB里是小写x 转大写会与stm32f1xx.h的defined()对不上
    DEVICE_MACRO="${MCU}"
elif [[ "${MCU_UC}" =~ ^STM32F[0-9]{3}[A-Z][0-9A-Z][A-Z][0-9]$ ]]; then
    DEVICE_MACRO="$(mcu_to_macro "${MCU_UC}")" || {
        echo "ERROR: unknown flash code in '${MCU_UC}'"
        exit 1
    }
else
    echo "ERROR: cannot parse MCU '${MCU}'. Use e.g. STM32F103C8T6 or a density code like STM32F103xB"
    exit 1
fi

# 由宏推导出 启动文件/链接脚本(命名规则和ST包一致) STM32F103xB->startup_stm32f103xb.s/STM32F103XB_FLASH.ld
DEVICE_UC="$(printf '%s' "${DEVICE_MACRO}" | tr '[:lower:]' '[:upper:]' | tr -d '.')"
DEVICE_LC="$(printf '%s' "${DEVICE_MACRO}" | tr '[:upper:]' '[:lower:]' | tr -d '.')"
STARTUP_FILE="startup_${DEVICE_LC}.s"
LINKER_SCRIPT="${DEVICE_UC}_FLASH.ld"

echo "Device:      ${DEVICE_MACRO}"
echo "Startup:     ${STARTUP_FILE}"
echo "Linker:      ${LINKER_SCRIPT}"

# cmake检查
command -v cmake >/dev/null 2>&1 || { echo "ERROR: cmake not found. Install: sudo apt install cmake"; exit 1; }
# 交叉编译器检查
command -v arm-none-eabi-gcc >/dev/null 2>&1 || { echo "ERROR: arm-none-eabi-gcc not found. Install: sudo apt install gcc-arm-none-eabi"; exit 1; }

# hal
if [ ! "$(ls -A "${SCRIPT_DIR}/lib" 2>/dev/null)" ]; then
    echo "lib/ is empty, copying HAL driver from ST package..."
    mkdir -p "${SCRIPT_DIR}/lib/Inc" "${SCRIPT_DIR}/lib/Src"
    cp -r "${ST_MCU_PATH}/Drivers/STM32F1xx_HAL_Driver/Inc/." "${SCRIPT_DIR}/lib/Inc/"
    cp -r "${ST_MCU_PATH}/Drivers/STM32F1xx_HAL_Driver/Src/." "${SCRIPT_DIR}/lib/Src/"
fi

# hal配置头文件(模板拷贝并裁剪 只用其中一部分模块时手动精简)
if [ ! -f "${SCRIPT_DIR}/lib/Inc/stm32f1xx_hal_conf.h" ]; then
    echo "stm32f1xx_hal_conf.h not found, copying from template..."
    cp "${SCRIPT_DIR}/lib/Inc/stm32f1xx_hal_conf_template.h" "${SCRIPT_DIR}/lib/Inc/stm32f1xx_hal_conf.h"
fi

# cmsis 内核相关
if [ ! "$(ls -A "${SCRIPT_DIR}/cmsis" 2>/dev/null)" ]; then
    echo "cmsis/ is empty, copying CMSIS from ST package..."
    mkdir -p "${SCRIPT_DIR}/cmsis/Include" "${SCRIPT_DIR}/cmsis/Src"
    cp -r "${ST_MCU_PATH}/Drivers/CMSIS/Include/." "${SCRIPT_DIR}/cmsis/Include/"
    cp -r "${ST_MCU_PATH}/Drivers/CMSIS/Device/ST/STM32F1xx/Include/." "${SCRIPT_DIR}/cmsis/Include/"
    cp "${ST_MCU_PATH}/Drivers/CMSIS/Device/ST/STM32F1xx/Source/Templates/system_stm32f1xx.c" "${SCRIPT_DIR}/cmsis/Src/"
fi

# 启动文件(按型号 gcc编译器用Templates/gcc下的启动文件 arm/是keil版的)
if [ ! -f "${SCRIPT_DIR}/cmsis/Src/${STARTUP_FILE}" ]; then
    echo "startup file not found, copying from ST package..."
    cp "${ST_MCU_PATH}/Drivers/CMSIS/Device/ST/STM32F1xx/Source/Templates/gcc/${STARTUP_FILE}" "${SCRIPT_DIR}/cmsis/Src/"
fi

# hw 中断相关文件
if [ ! "$(ls -A "${SCRIPT_DIR}/hw" 2>/dev/null)" ]; then
    echo "hw/ is empty, copying interrupt handlers from ST package..."
    mkdir -p "${SCRIPT_DIR}/hw/Inc" "${SCRIPT_DIR}/hw/Src"
    cp "${ST_MCU_PATH}/Projects/${BOARD_EXAMPLE}/Examples/UART/UART_TwoBoards_ComIT/Inc/stm32f1xx_it.h" "${SCRIPT_DIR}/hw/Inc/"
    cp "${ST_MCU_PATH}/Projects/${BOARD_EXAMPLE}/Examples/UART/UART_TwoBoards_ComIT/Src/stm32f1xx_it.c" "${SCRIPT_DIR}/hw/Src/"
fi

# 链接脚本(和上面的启动文件/设备宏对应)
if [ ! -f "${SCRIPT_DIR}/${LINKER_SCRIPT}" ]; then
    echo "linker script not found, copying from ST package..."
    cp "${ST_MCU_PATH}/Drivers/CMSIS/Device/ST/STM32F1xx/Source/Templates/gcc/linker/${LINKER_SCRIPT}" "${SCRIPT_DIR}/"
fi

# build
echo "Configuring build..."
mkdir -p "${BUILD_DIR}"
cmake -B "${BUILD_DIR}" -S . \
    -DST_MCU_PATH="${ST_MCU_PATH}" \
    -DDEVICE_MACRO="${DEVICE_MACRO}" \
    -DSTARTUP_FILE="cmsis/Src/${STARTUP_FILE}" \
    -DLINKER_SCRIPT="${LINKER_SCRIPT}"

echo "Building..."
cmake --build "${BUILD_DIR}" -j$(nproc)

echo ""
echo "=== Build complete ==="
echo "Artifacts:"
ls -la "${BUILD_DIR}/stm32" "${BUILD_DIR}/stm32.hex" "${BUILD_DIR}/stm32.bin"
echo ""
echo "Flash with ST-Link:"
echo "  st-flash write ${BUILD_DIR}/stm32.bin 0x08000000"
