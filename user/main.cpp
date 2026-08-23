//
// Created by dingrui on 8/23/26.
//

#include "stm32f1xx_hal.h"

#include "rcc.h"

int main() {
    // 必须最先调用
    HAL_Init();

    // 倍频
    RccClock_Init();

    // 系统时钟波形输出到PA8
    HAL_RCC_MCOConfig(RCC_MCO1, RCC_MCO1SOURCE_SYSCLK, RCC_MCODIV_1);

    SystemCoreClockUpdate();
    uint32_t HCLKFreq = HAL_RCC_GetHCLKFreq();
    uint32_t PCLK1Freq = HAL_RCC_GetPCLK1Freq();
    uint32_t PCLK2Freq = HAL_RCC_GetPCLK2Freq();
}
