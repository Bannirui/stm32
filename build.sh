#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"

# 芯片型号 用命令行参数指定 例如./build.sh STM32F103C8T6
# 也可以直接给密度档位宏 例如./build.sh STM32F103xB
MCU="${1:-stm32f103c8t6}"
MCU_UC="$(printf '%s' "${MCU}" | tr '[:lower:]' '[:upper:]')"
# st的mcu package 可用环境变量覆盖(也传给cmake 本地没有时才走FetchContent)
ST_MCU_PATH="${ST_MCU_PATH:-/home/dingrui/MyApp/st/package/STM32Cube_FW_F1_V1.8.0}"
# 兜底用的ST示例工程目录名(it文件缺失时才从该示例拷贝 不同板子路径不同 可用环境变量覆盖)
BOARD_EXAMPLE="${BOARD_EXAMPLE:-STM32F103RB-Nucleo}"

# 按ST命名规则解析 STM32F{家族}{引脚}{Flash容量}{封装}{温度}
# 其中Flash容量位(C8T6的8)决定密度档位宏 映射关系与stm32f1xx.h一致
# 4/6映射x6 8/B映射xB C/D/E映射xE F/G映射xG F105/107映射xC
# 直接传密度档位时保留原大小写(STM32F103xB的小写x必须保留 否则与stm32f1xx.h的defined()对不上 编译器会当成F105系列)
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

if [[ "${MCU_UC}" =~ ^STM32F[0-9]{3}[xX][A-Z0-9]$ ]]; then
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

echo "Device:      ${DEVICE_MACRO}"
echo "ST package:  ${ST_MCU_PATH}"

command -v cmake >/dev/null 2>&1 || { echo "ERROR: cmake not found. Install: sudo apt install cmake"; exit 1; }
command -v arm-none-eabi-gcc >/dev/null 2>&1 || { echo "ERROR: arm-none-eabi-gcc not found. Install: sudo apt install gcc-arm-none-eabi"; exit 1; }

# hw下的中断文件(工程私有源码 正常应提交在git里)
# 兜底:文件缺失时才从ST示例拷贝最小可编译版(功能阉割 无EXTI处理)并提示用git恢复完整版
if [ ! -f "${SCRIPT_DIR}/hw/stm32f1xx_it.c" ] || [ ! -f "${SCRIPT_DIR}/hw/stm32f1xx_it.h" ]; then
    echo "WARN: stm32f1xx_it files missing, copying fallback from ST example..."
    mkdir -p "${SCRIPT_DIR}/hw"
    cp "${ST_MCU_PATH}/Projects/${BOARD_EXAMPLE}/Examples/UART/UART_TwoBoards_ComIT/Inc/stm32f1xx_it.h" "${SCRIPT_DIR}/hw/"
    cp "${ST_MCU_PATH}/Projects/${BOARD_EXAMPLE}/Examples/UART/UART_TwoBoards_ComIT/Src/stm32f1xx_it.c" "${SCRIPT_DIR}/hw/"
    # ST示例依赖其工程里的main.h/UartHandle/USER_BUTTON_PIN 本工程没有 裁剪掉
    sed -i '/#include "main.h"/d' "${SCRIPT_DIR}/hw/stm32f1xx_it.c"
    sed -i '/extern UART_HandleTypeDef UartHandle;/d' "${SCRIPT_DIR}/hw/stm32f1xx_it.c"
    sed -i 's/HAL_UART_IRQHandler(&UartHandle);/\/\* HAL_UART_IRQHandler(\&UartHandle); \*\//' "${SCRIPT_DIR}/hw/stm32f1xx_it.c"
    sed -i 's/HAL_GPIO_EXTI_IRQHandler(USER_BUTTON_PIN);/\/\* HAL_GPIO_EXTI_IRQHandler(USER_BUTTON_PIN); \*\//' "${SCRIPT_DIR}/hw/stm32f1xx_it.c"
    echo "WARN: fallback written (basic IRQ handlers only)."
    echo "      For the full version (PC13/PA0 EXTI), restore from git:"
    echo "      git restore hw/stm32f1xx_it.c hw/stm32f1xx_it.h"
fi

# build(启动文件/链接脚本由CMakeLists根据DEVICE_MACRO自动推导)
echo "Configuring build..."
mkdir -p "${BUILD_DIR}"
cmake -B "${BUILD_DIR}" -S . \
    -DST_MCU_PATH="${ST_MCU_PATH}" \
    -DDEVICE_MACRO="${DEVICE_MACRO}"

echo "Building..."
cmake --build "${BUILD_DIR}" -j$(nproc)

echo ""
echo "=== Build complete ==="
echo "Artifacts:"
ls -la "${BUILD_DIR}/stm32" "${BUILD_DIR}/stm32.hex" "${BUILD_DIR}/stm32.bin"
echo ""
echo "Flash with ST-Link:"
echo "  st-flash write ${BUILD_DIR}/stm32.bin 0x08000000"
