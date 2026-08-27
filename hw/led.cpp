//
// Created by dingrui on 8/23/26.
//

#include "led.h"

#include <stm32f1xx_hal.h>

// 开时钟
static void enableGpioClock(GPIO_TypeDef *port) {
    if (port == GPIOA) {
        __HAL_RCC_GPIOA_CLK_ENABLE();
    } else if (port == GPIOB) {
        __HAL_RCC_GPIOB_CLK_ENABLE();
    } else if (port == GPIOC) {
        __HAL_RCC_GPIOC_CLK_ENABLE();
    } else if (port == GPIOD) {
        __HAL_RCC_GPIOD_CLK_ENABLE();
    } else if (port == GPIOE) {
        __HAL_RCC_GPIOE_CLK_ENABLE();
    }
}

Led::Led(GPIO_TypeDef *port, uint16_t pin)
    : port(port), pin(pin) {
    enableGpioClock(port);

    GPIO_InitTypeDef init{};
    init.Pin = pin;
    // 推挽输出
    init.Mode = GPIO_MODE_OUTPUT_PP;
    // 驱动led不需要高频率
    init.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(port, &init);

    // 给低电平 灯灭
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
}

void Led::toggleMs(unsigned long ms) {
    if (ms>0) {
        HAL_Delay(ms);
    }
    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
}
