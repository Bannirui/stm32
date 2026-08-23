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

    while (1) {
        // sw8按键被按下执行灯亮
        switch (sw_scan(0)) {
            case 8:
                HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
                break;
        }
    }
}
