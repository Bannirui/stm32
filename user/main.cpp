//
// Created by dingrui on 8/23/26.
//

#include "stm32f1xx_hal.h"

#include "rcc.h"
#include "led.h"
#include "sw.h"
#include "uart.h"

int main() {
    // 必须最先调用
    HAL_Init();

    // 时钟配置
    RccLock72 rcc;

    // PC13
    Led led(GPIOC, GPIO_PIN_13);

    // 按键 扫描 PC13轮询
    sw8_scan.initInput(GPIO_PULLDOWN);
    // 按键 中断 PC13按下触发
    sw8_it.initIt(Sw::Edge::Press, GPIO_PULLDOWN, 4);
    // 按键 中断 PA0抬起触发
    sw11_it.initIt(Sw::Edge::Release, GPIO_PULLUP, 3);

    // IO口锁定
    HAL_GPIO_LockPin(GPIOA, GPIO_PIN_0);

    // IO口复位 复位成浮空输入模式
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_0);

    // 串口uart1
    Uart uart1(USART1, 921600);
    // 启动中断接收 收到的字节自动写入环形缓冲
    uart1.startRx();

    // LED闪烁计数
    uint32_t ledTick = 0;

    while (1) {
        // uart收到什么返回什么
        uint8_t b;
        while (uart1.read(b)) {
            uart1.send(&b, 1);
        }

        // sw8按键被按下执行灯亮
        switch (sw8_scan.scan(Sw::Edge::Press)) {
            case SwScan::Result::Pressed:
                led.toggleMs(0);
                break;
            default:
                break;
        }

        // LED每秒闪一次 用计数代替阻塞延时 保证串口回显及时
        if (++ledTick % 1000 == 0) {
            led.toggleMs(0);
            // 软件触发中断
            __HAL_GPIO_EXTI_GENERATE_SWIT(GPIO_PIN_13);
        }

        HAL_Delay(1);
    }
}