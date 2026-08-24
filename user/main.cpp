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
    RccLock72 rcc;

    // PC13
    Led led(GPIOC, GPIO_PIN_13);

    // 按键 扫描 PC13轮询
    sw8_scan.initInput(GPIO_PULLDOWN);
    // 按键 中断 PC13按下触发
    sw8_it.initIt(Sw::Edge::Press, GPIO_PULLDOWN, 4);
    // 按键 中断 PA0抬起触发
    sw11_it.initIt(Sw::Edge::Release, GPIO_PULLUP, 3);

    while (1) {
        led.toggleMs(1000);
        // sw8按键被按下执行灯亮
        switch (sw8_scan.scan(Sw::Edge::Press)) {
            case SwScan::Result::Pressed:
                led.toggleMs(0);
                break;
            default:
                break;
        }
        HAL_Delay(1000);
        // 软件触发中断
        __HAL_GPIO_EXTI_GENERATE_SWIT(GPIO_PIN_13);
    }
}
