//
// Created by dingrui on 8/23/26.
//

#include "stm32f1xx_hal.h"

#include "rcc.h"
#include "led.h"
#include "sw.h"

int main() {
    // 必须最先调用
    HAL_Init();

    // 倍频
    RccClock_Init();

    led_init();

    sw_init_it(0);

    while (1) {
    }
}
