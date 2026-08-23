//
// Created by dingrui on 8/23/26.
//

#pragma once

#include <stdint.h>

// 按键8控制PC11
#define SW8_IN HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13)
// 按键11控制PA0
#define SW11_IN HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0)


// 按键
void sw_init();

/**
 * 按键中断
 * @param mode 0表示按键按下执行 1表示按键抬起执行
 */
void sw_init_it(uint8_t mode);

/**
 * 检测按键被按下或者抬起 检测到动作
 * @param mode 0表示按下 1表示抬起
 * @return 0表示没有按键触发 n表示swn触发
 */
uint8_t sw_scan(uint8_t mode);
