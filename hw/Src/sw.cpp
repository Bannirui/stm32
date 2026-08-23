//
// Created by dingrui on 8/23/26.
//

#include "sw.h"

#include "stm32f1xx_hal.h"

void sw_init() {
    // 开C口的时钟
    __HAL_RCC_GPIOC_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitType;
    GPIO_InitType.Pin = GPIO_PIN_13;
    // 输入
    GPIO_InitType.Mode = GPIO_MODE_AF_INPUT;
    // 下拉
    GPIO_InitType.Pull = GPIO_PULLDOWN;
    // PC13
    HAL_GPIO_Init(GPIOC, &GPIO_InitType);
}

void sw_init_it(uint8_t mode) {
    // 开C口的时钟
    __HAL_RCC_GPIOC_CLK_ENABLE();
    GPIO_InitTypeDef PC13_InitType;
    // PC13
    PC13_InitType.Pin = GPIO_PIN_13;
    if (mode == 0) {
        // 按下执行 上升沿
        PC13_InitType.Mode = GPIO_MODE_IT_RISING;
    } else {
        // 抬起执行 下降沿
        PC13_InitType.Mode = GPIO_MODE_IT_FALLING;
    }
    PC13_InitType.Pull = GPIO_PULLDOWN;
    // PC13
    HAL_GPIO_Init(GPIOC, &PC13_InitType);
    // 中断优先级
    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 4, 0);
    // 使能
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

    // PA
    __HAL_RCC_GPIOC_CLK_ENABLE();
    GPIO_InitTypeDef PA0_InitType;
    // PA0
    PA0_InitType.Pin = GPIO_PIN_0;
    if (mode == 0) {
        // 按下执行 下降沿
        PA0_InitType.Mode = GPIO_MODE_IT_FALLING;
    } else {
        // 抬起执行 上升沿
        PA0_InitType.Mode = GPIO_MODE_IT_RISING;
    }
    PA0_InitType.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOA, &PA0_InitType);
    // 中断优先级
    HAL_NVIC_SetPriority(EXTI0_IRQn, 3, 0);
    // 使能
    HAL_NVIC_EnableIRQ(EXTI0_IRQn);
}

// 状态 0表示没有按下 1表示被按下
uint8_t sw8_state;

uint8_t sw_scan(uint8_t mode) {
    uint32_t i;
    if ((SW8_IN == 1) && (sw8_state == 0)) {
        // 检测到按键被按下 原来按键是抬起的并且现在被按下
        // 延时消抖
        for (i = 0; i < 0x7fff; i++) {
            if (SW8_IN == 0) {
                return 0;
            }
            // 稳定保持在高电平 说明按键被按下了
            sw8_state = 1;
            if (mode == 0) {
                return 8;
            }
        }
    } else if ((SW8_IN == 0) && (sw8_state == 1)) {
        // 检测按键被抬起 原来的按键是按下的 现在被抬起
        // 延时消抖
        for (i = 0; i < 0x7fff; i++) {
            if (SW8_IN == 1) {
                return 0;
            }
            // 稳定保持在低电平 说明按键被抬起了
            sw8_state = 0;
            if (mode == 1) {
                return 8;
            }
        }
    }
    return 0;
}

// PC13跟PA0的回调都会进到这个函数
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    switch (GPIO_Pin) {
        case GPIO_PIN_13: {
            // PC13->sw8
            if (SW8_IN == 1) {
                // 延时消抖
                for (uint32_t i = 0; i < 0x7fff; i++) {
                    if (SW8_IN == 0) {
                        // 可能是干扰 也可能是短按
                        return;
                    }
                }
                HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_0);
            } else if (SW8_IN == 0) {
                // 延时消抖
                for (uint32_t i = 0; i < 0x7fff; i++) {
                    if (SW8_IN == 1) {
                        // 可能是干扰 也可能是不满足长短按要求
                        return;
                    }
                }
                HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_0);
            }
            break;
        }
        case GPIO_PIN_0: {
            // PA0->sw11 高优先级
            if (SW11_IN == 0) {
                // 延时消抖
                for (uint32_t i = 0; i < 0x7fff; i++) {
                    if (SW11_IN == 1) {
                        // 可能是干扰 也可能是没有满足长短按要求
                        return;
                    }
                }
                HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_0);
            } else if (SW11_IN == 1) {
                // 延时消抖
                for (uint32_t i = 0; i < 0x7fff; i++) {
                    if (SW11_IN == 0) {
                        // 可能是干扰 也可能是没有满足长短按要求
                        return;
                    }
                }
                HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_0);
            }
            break;
        }
        default: break;
    }
}
