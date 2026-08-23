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
