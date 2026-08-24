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

# 设备相关:和启动文件/链接脚本保持一致(可由build.sh通过-D覆盖)
set(DEVICE_MACRO  "STM32F103xB"                  CACHE STRING "STM32 device macro")
set(STARTUP_FILE  "cmsis/Src/startup_stm32f103xb.s" CACHE STRING "Startup assembly file")
set(LINKER_SCRIPT "STM32F103XB_FLASH.ld"          CACHE STRING "Linker script")

# 编译选项
add_compile_definitions(${DEVICE_MACRO})
add_compile_options(-mcpu=cortex-m3 -mthumb -ffunction-sections -fdata-sections -Wall)
add_compile_options(-Os -g3)

# lib/cmsis是ST库 基本不动 用显式列表(可靠可重复)
# hw/user是工程代码 经常新增 用GLOB自动收集(新增文件无需改cmake)
file(GLOB_RECURSE PROJECT_APP_SOURCES CONFIGURE_DEPENDS
        hw/*.c
        hw/*.cpp
        user/*.c
        user/*.cpp
)
list(FILTER PROJECT_APP_SOURCES EXCLUDE REGEX "_template\\.c$")

file(GLOB_RECURSE LIB_SOURCES CONFIGURE_DEPENDS lib/Src/*.c)
list(FILTER LIB_SOURCES EXCLUDE REGEX "_template\\.c$")

set(CMSIS_SOURCES
        cmsis/Src/system_stm32f1xx.c
        ${STARTUP_FILE}
)

add_executable(${PROJECT_NAME}
        ${LIB_SOURCES}
        ${CMSIS_SOURCES}
        ${PROJECT_APP_SOURCES}
)

# 头文件路径
target_include_directories(${PROJECT_NAME} PRIVATE
        cmsis/Include
        hw/Inc
        lib/Inc
        user
)

# 链接
target_link_options(stm32 PRIVATE
        -mcpu=cortex-m3 -mthumb
        -T ${CMAKE_SOURCE_DIR}/${LINKER_SCRIPT}
        -Wl,--gc-sections
        --specs=nano.specs --specs=nosys.specs
)

# 生成hex和bin
add_custom_command(TARGET stm32 POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:stm32> ${CMAKE_BINARY_DIR}/stm32.hex
        COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:stm32> ${CMAKE_BINARY_DIR}/stm32.bin
)
