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

}
