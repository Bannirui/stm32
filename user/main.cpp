//
// Created by dingrui on 8/23/26.
//

#include "stm32f1xx_hal.h"

int main() {
    HAL_Init();
    // 系统时钟波形输出到PA8
    HAL_RCC_MCOConfig(RCC_MCO1, RCC_MCO1SOURCE_SYSCLK, RCC_MCODIV_1);
}
