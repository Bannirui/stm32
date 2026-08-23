//
// Created by dingrui on 8/23/26.
//

#pragma once

#include <stdint.h>

#define SW8_IN HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13)


// 按键
void sw_init();

/**
 * 检测按键被按下或者抬起 检测到动作
 * @param mode 0表示按下 1表示抬起
 * @return 0表示没有按键触发 n表示swn触发
 */
uint8_t sw_scan(uint8_t mode);
