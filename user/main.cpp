//
// Created by dingrui on 8/23/26.
//

#include "stm32f1xx_hal.h"

#include <stdio.h>

#include "rcc.h"
#include "led.h"
#include "sw.h"
#include "uart.h"
#include "dbg_printf.h"

uint8_t b_dma;

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

    // 轮询 阻塞接收 没有后台
    UartBase uart1_poll(USART1, 115200);

    // 串口调试 printf/puts从这里输出
    dbg_printf_init(&uart1_poll);
    printf("uart1 init @ 115200\r\n");

    // 中断 逐字节中断 自动进环形缓冲
    UartInterrupt uart1_it(USART1, 921600);
    uart1_it.startRx();

    // DMA 环形DMA+空闲中断 自动进环形缓冲
    UartDma uart1_dma(USART1, 921600);
    uint8_t dma_buf[256];
    uart1_dma.startRxDma(dma_buf, sizeof(dma_buf));

    // LED闪烁计数
    uint32_t ledTick = 0;

    while (1) {
        led.toggleMs(1000);

        // uart收到什么返回什么
        uint8_t b_poll;
        uint8_t b_it;

        // 轮询:阻塞等1字节 超时100ms
        if (uart1_poll.receive(&b_poll, 1, 100) == HAL_OK) {
            uart1_poll.send(&b_poll, 1);
        }

        // 中断/DMA:从环形缓冲取
        while (uart1_it.read(b_it)) {
            uart1_it.send(&b_it, 1);
        }

        while (uart1_dma.read(b_dma)) {
            uart1_dma.send(&b_dma, 1);
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
