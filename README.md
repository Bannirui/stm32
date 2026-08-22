stm32
---

learn stm32, including HAL

- 1 [download HAL from st](https://www.st.com/en/embedded-software/stm32cube-mcu-mpu-packages.html) and the local path `/home/dingrui/MyApp/st/HAL`
- 2 库相关 拷贝include和src 放到lib下
  - `STM32Cube_FW_F1_V1.8.0/Drivers/STM32F1xx_HAL_Driver/Inc`
  - `STM32Cube_FW_F1_V1.8.0/Drivers/STM32F1xx_HAL_Driver/Src`
- 3 内核相关 
  - 头文件放到cmsis下的Include
    - 拷贝 `STM32Cube_FW_F1_V1.8.0/Drivers/CMSIS/Include`
    - 拷贝 `STM32Cube_FW_F1_V1.8.0/Drivers/CMSIS/Device/ST/STM32F1xx/Include`
  - 源文件放到cmsis下的Src
    - `STM32Cube_FW_F1_V1.8.0/Drivers/CMSIS/Device/ST/STM32F1xx/Source/Templates/system_stm32f1xx`
    - 汇编的启动文件 `STM32Cube_FW_F1_V1.8.0/Drivers/CMSIS/Device/ST/STM32F1xx/Source/Templates/arm`

## QUICK START

```sh
./build.sh
```