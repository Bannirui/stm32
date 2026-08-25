//
// Created by dingrui on 8/23/26.
//

#include "stm32f1xx_hal.h"

#include "rcc.h"
#include "led.h"
#include "sw.h"
#include "uart.h"

// uart一次接收200个字节
#define RX_SIZE 200

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

    // uart的缓冲区
    uint8_t buf[1024];
    // 串口uart1
    Uart uart1(USART1, 921600);

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

        // uart收到什么返回什么
        switch (uart1.receive(buf, RX_SIZE, 500)) {
            case HAL_OK:
                uart1.send(buf, RX_SIZE);
                break;
            case HAL_TIMEOUT: {
                // 实际收到的字节数
                uint16_t n = RX_SIZE - uart1.handlePtr()->RxXferCount;
                if (n > 0) {
                    uart1.send(buf, n);
                }
                break;
            }
            default:
                break;
        }
    }
}
