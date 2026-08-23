stm32
---

learn stm32, including HAL

- 1 [download HAL from st](https://www.st.com/en/embedded-software/stm32cube-mcu-mpu-packages.html) and the local path `/home/dingrui/MyApp/st/HAL`
- 2 库相关 放到lib下
  - `STM32Cube_FW_F1_V1.8.0/Drivers/STM32F1xx_HAL_Driver/Inc`
  - `STM32Cube_FW_F1_V1.8.0/Drivers/STM32F1xx_HAL_Driver/Src`
- 3 内核相关 放到cmsis下 
  - cmsis下的Include
    - `STM32Cube_FW_F1_V1.8.0/Drivers/CMSIS/Include`
    - `STM32Cube_FW_F1_V1.8.0/Drivers/CMSIS/Device/ST/STM32F1xx/Include`
  - 源文件放到cmsis下的Src
    - `STM32Cube_FW_F1_V1.8.0/Drivers/CMSIS/Device/ST/STM32F1xx/Source/Templates/system_stm32f1xx`
    - 汇编的启动文件 `STM32Cube_FW_F1_V1.8.0/Drivers/CMSIS/Device/ST/STM32F1xx/Source/Templates/arm`
- 4 中断相关 放到hw下
  - `/home/dingrui/MyApp/st/package/STM32Cube_FW_F1_V1.8.0/Projects/STM32F103RB-Nucleo/Examples/UART/UART_TwoBoards_ComIT/Inc/stm32f1xx_it.h`
  - `/home/dingrui/MyApp/st/package/STM32Cube_FW_F1_V1.8.0/Projects/STM32F103RB-Nucleo/Examples/UART/UART_TwoBoards_ComIT/Src/stm32f1xx_it.c`

## QUICK START

```sh
./build.sh                     # 默认stm32f103c8t6
./build.sh stm32f103c8t6       # 你的芯片
./build.sh STM32F103ZET6       # 大容量
./build.sh STM32F103xB         # 也可直接给密度档位
```