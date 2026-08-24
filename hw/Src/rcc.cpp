//
// Created by dingrui on 8/23/26.
//

#include "rcc.h"

#include "stm32f1xx_hal.h"

Rcc::Rcc(Source source) {
    // 振荡器
    RCC_OscInitTypeDef osc{};
    osc.PLL.PLLState = RCC_PLL_ON;
    if (source == Source::Hsi) {
        osc.OscillatorType = RCC_OSCILLATORTYPE_HSI;
        osc.HSIState = RCC_HSI_ON;
        osc.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
        osc.PLL.PLLSource = RCC_PLLSOURCE_HSI_DIV2;
        osc.PLL.PLLMUL = RCC_PLL_MUL16;
    } else {
        osc.OscillatorType = RCC_OSCILLATORTYPE_HSE;
        osc.HSEState = RCC_HSE_ON;
        osc.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
        osc.PLL.PLLSource = RCC_PLLSOURCE_HSE;
        osc.PLL.PLLMUL = RCC_PLL_MUL9;
    }
    HAL_RCC_OscConfig(&osc);

    // 总线分频（两模式相同）
    RCC_ClkInitTypeDef clk{};
    clk.ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV2;
    clk.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_2);
}

RccLock64::RccLock64() : Rcc(Source::Hsi) {
}

RccLock72::RccLock72() : Rcc(Source::Hse) {
}
