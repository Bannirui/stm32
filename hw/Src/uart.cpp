//
// Created by dingrui on 8/25/26.
//

#include "uart.h"

#include "stm32f1xx_hal.h"

#include <string.h>

// uart1/2/3的回调都会到这个函数 串口引脚、时钟 在MspInit中按实例分发
void HAL_UART_MspInit(UART_HandleTypeDef *huart) {
    GPIO_InitTypeDef init{};
    if (huart->Instance == USART1) {
        // 打开串口1时钟
        __HAL_RCC_USART1_CLK_ENABLE();
        // 打开A口时钟
        __HAL_RCC_GPIOA_CLK_ENABLE();
        init.Pin = GPIO_PIN_9; // PA9 TX 发送引脚
        init.Mode = GPIO_MODE_AF_PP; // 输出方向
        init.Speed = GPIO_SPEED_FREQ_HIGH; // 速度
        HAL_GPIO_Init(GPIOA, &init);

        init.Pin = GPIO_PIN_10;   // PA10 RX 接收引脚
        init.Mode = GPIO_MODE_AF_INPUT; // 输入方向
        init.Pull = GPIO_NOPULL; // 浮空 没有上拉也没有下拉
        HAL_GPIO_Init(GPIOA, &init);
    } else if (huart->Instance == USART2) {
        __HAL_RCC_USART2_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();
        init.Pin = GPIO_PIN_2; // PA2 TX
        init.Mode = GPIO_MODE_AF_PP;
        init.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(GPIOA, &init);

        init.Pin = GPIO_PIN_3; // PA3 RX
        init.Mode = GPIO_MODE_AF_INPUT;
        init.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOA, &init);
    } else if (huart->Instance == USART3) {
        __HAL_RCC_USART3_CLK_ENABLE();
        __HAL_RCC_GPIOB_CLK_ENABLE();
        init.Pin = GPIO_PIN_10; // PB10 TX
        init.Mode = GPIO_MODE_AF_PP;
        init.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(GPIOB, &init);

        init.Pin = GPIO_PIN_11; // PB11 RX
        init.Mode = GPIO_MODE_AF_INPUT;
        init.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOB, &init);
    }
}

Uart::Uart(USART_TypeDef *instance, uint32_t baud) {
    handle.Instance = instance;
    handle.Init.BaudRate = baud;
    handle.Init.WordLength = UART_WORDLENGTH_8B;
    handle.Init.StopBits = UART_STOPBITS_1;
    handle.Init.Parity = UART_PARITY_NONE;
    handle.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    handle.Init.Mode = UART_MODE_TX_RX;
    HAL_UART_Init(&handle);
}

void Uart::send(const uint8_t *data, uint16_t len) {
    HAL_UART_Transmit(&handle, const_cast<uint8_t *>(data), len, HAL_MAX_DELAY);
}

void Uart::send(const char *str) {
    send(reinterpret_cast<const uint8_t *>(str), static_cast<uint16_t>(strlen(str)));
}

HAL_StatusTypeDef Uart::receive(uint8_t *data, uint16_t len, uint32_t timeout) {
    HAL_StatusTypeDef status = HAL_UART_Receive(&handle, data, len, timeout);
    if (status == HAL_TIMEOUT) {
        // 超时返回后 HAL 不会复位状态也不会解锁 会导致下一次调用直接返回 HAL_BUSY
        handle.gState = HAL_UART_STATE_READY;
        handle.RxState = HAL_UART_STATE_READY;
        handle.Lock = HAL_UNLOCKED;
        handle.ErrorCode = HAL_UART_ERROR_NONE;
    }
    return status;
}

UART_HandleTypeDef *Uart::handlePtr() {
    return &handle;
}
