cmake_minimum_required(VERSION 3.13)

# 交叉编译工具链(必须在project之前设置 否则CMake不识别为交叉编译)
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)
set(CMAKE_EXECUTABLE_SUFFIX_ELF elf)
# 裸机没有crt/入口 编译器检测时只编译不链接
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

project(stm32 LANGUAGES C CXX ASM)

# ---------- ST库来源 ----------
# 优先用本地STM32Cube_FW_F1 package路径不存在时才用FetchContent下载
set(ST_MCU_PATH "/home/dingrui/MyApp/st/package/STM32Cube_FW_F1_V1.8.0"
        CACHE PATH "Local STM32Cube_FW_F1 path; leave empty to download via FetchContent")

if(EXISTS "${ST_MCU_PATH}/Drivers/STM32F1xx_HAL_Driver/Inc")
    set(STM32_FW_ROOT "${ST_MCU_PATH}")
    message(STATUS "STM32Cube FW: using local package ${STM32_FW_ROOT}")
else()
    include(FetchContent)
    FetchContent_Declare(
        stm32cube_fw_f1
        GIT_REPOSITORY https://github.com/STMicroelectronics/STM32CubeF1.git
        GIT_TAG        v1.8.0
    )
    FetchContent_GetProperties(stm32cube_fw_f1)
    if(NOT stm32cube_fw_f1_POPULATED)
        FetchContent_Populate(stm32cube_fw_f1)
    endif()
    set(STM32_FW_ROOT "${stm32cube_fw_f1_SOURCE_DIR}")
    message(STATUS "STM32Cube FW: downloaded to ${STM32_FW_ROOT}")
endif()

# 设备相关:宏、启动文件、链接脚本三者由DEVICE_MACRO统一推导 保持一致性
set(DEVICE_MACRO "STM32F103xB" CACHE STRING "STM32 device macro")
string(TOLOWER "${DEVICE_MACRO}" _device_lc)
string(TOUPPER "${DEVICE_MACRO}" _device_uc)

# 启动文件
set(STARTUP_FILE "${STM32_FW_ROOT}/Drivers/CMSIS/Device/ST/STM32F1xx/Source/Templates/gcc/startup_${_device_lc}.s")
# 链接脚本
set(LINKER_SCRIPT "${STM32_FW_ROOT}/Drivers/CMSIS/Device/ST/STM32F1xx/Source/Templates/gcc/linker/${_device_uc}_FLASH.ld")

# ---------- HAL配置头文件 ----------
# 直接从ST包的模板拷贝到build目录 改名stm32f1xx_hal_conf.h
# (COPYONLY=纯拷贝 内容未变不重写) 源文件始终来自package 工程里无需再维护一份
set(HAL_CONF_GEN_DIR "${CMAKE_BINARY_DIR}/generated/config")
file(MAKE_DIRECTORY "${HAL_CONF_GEN_DIR}")
configure_file(
        "${STM32_FW_ROOT}/Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal_conf_template.h"
        "${HAL_CONF_GEN_DIR}/stm32f1xx_hal_conf.h"
        COPYONLY)

# 编译选项
add_compile_definitions(${DEVICE_MACRO})
add_compile_options(-mcpu=cortex-m3 -mthumb -ffunction-sections -fdata-sections -Wall)
add_compile_options(-Os -g3)

# hw/user是工程代码 经常新增 用GLOB自动收集(新增文件无需改cmake)
file(GLOB_RECURSE PROJECT_APP_SOURCES CONFIGURE_DEPENDS
        hw/*.c
        hw/*.cpp
        user/*.c
        user/*.cpp
)
list(FILTER PROJECT_APP_SOURCES EXCLUDE REGEX "_template\\.c$")

# HAL库源码直接来自package
file(GLOB LIB_SOURCES CONFIGURE_DEPENDS "${STM32_FW_ROOT}/Drivers/STM32F1xx_HAL_Driver/Src/*.c")
list(FILTER LIB_SOURCES EXCLUDE REGEX "_template\\.c$")

set(CMSIS_SOURCES
        "${STM32_FW_ROOT}/Drivers/CMSIS/Device/ST/STM32F1xx/Source/Templates/system_stm32f1xx.c"
        ${STARTUP_FILE}
)

add_executable(${PROJECT_NAME}
        ${LIB_SOURCES}
        ${CMSIS_SOURCES}
        ${PROJECT_APP_SOURCES}
)

# 头文件路径(顺序:生成的config在前 覆盖包内同名模板)
target_include_directories(${PROJECT_NAME} PRIVATE
        ${HAL_CONF_GEN_DIR}
        "${STM32_FW_ROOT}/Drivers/STM32F1xx_HAL_Driver/Inc"
        "${STM32_FW_ROOT}/Drivers/STM32F1xx_HAL_Driver/Inc/Legacy"
        "${STM32_FW_ROOT}/Drivers/CMSIS/Include"
        "${STM32_FW_ROOT}/Drivers/CMSIS/Device/ST/STM32F1xx/Include"
        hw
        user
)

# 链接
target_link_options(${PROJECT_NAME} PRIVATE
        -mcpu=cortex-m3 -mthumb
        -T ${LINKER_SCRIPT}
        -Wl,--gc-sections
        --specs=nano.specs --specs=nosys.specs
)

# 生成hex和bin
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${PROJECT_NAME}> ${CMAKE_BINARY_DIR}/stm32.hex
        COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}> ${CMAKE_BINARY_DIR}/stm32.bin
)
