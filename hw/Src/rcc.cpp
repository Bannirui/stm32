//
// Created by dingrui on 8/23/26.
//

#include "rcc.h"
#include "stm32f1xx_hal.h"

void RccClock_Init() {
    RCC_OscInitTypeDef Rcc_OscInitType;
    Rcc_OscInitType.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    Rcc_OscInitType.HSIState = RCC_HSI_ON;
    Rcc_OscInitType.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    Rcc_OscInitType.PLL.PLLState = RCC_PLL_ON;
    Rcc_OscInitType.PLL.PLLSource = RCC_PLLSOURCE_HSI_DIV2;
    Rcc_OscInitType.PLL.PLLMUL = RCC_PLL_MUL16;
    HAL_RCC_OscConfig(&Rcc_OscInitType);

    // 时钟源
    RCC_ClkInitTypeDef Rcc_ClkInitType;
    Rcc_ClkInitType.ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    Rcc_ClkInitType.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    Rcc_ClkInitType.AHBCLKDivider = RCC_SYSCLK_DIV1;
    Rcc_ClkInitType.APB1CLKDivider = RCC_HCLK_DIV2;
    Rcc_ClkInitType.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&Rcc_ClkInitType, FLASH_LATENCY_2);
}
