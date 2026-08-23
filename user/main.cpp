//
// Created by dingrui on 8/23/26.
//

#include "stm32f1xx_hal.h"

#include "rcc.h"
#include "led.h"

int main() {
    // 必须最先调用
    HAL_Init();

    // 倍频
    RccClock_Init();

    led_init();

    while (1) {
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        HAL_Delay(1000);
    }
}
