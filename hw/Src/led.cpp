//
// Created by dingrui on 8/23/26.
//

#include "led.h"

#include "stm32f1xx_hal.h"

void led_init() {
    // 开C口的时钟
    __HAL_RCC_GPIOC_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitType;
    GPIO_InitType.Pin = GPIO_PIN_13;
    // 推挽输出
    GPIO_InitType.Mode = GPIO_MODE_OUTPUT_PP;
    // 驱动led不需要高频率
    GPIO_InitType.Speed = GPIO_SPEED_FREQ_LOW;
    // PC13
    HAL_GPIO_Init(GPIOC, &GPIO_InitType);

    // 给低电平 灯灭
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
}
