//
// Created by dingrui on 8/25/26.
//

#pragma once

#include <stdint.h>

#include "stm32f1xx_hal.h"

// 8n1
class Uart {
public:
    /**
     * @param instance 决定是uart1/2/3
     * @param baud 波特率
     */
    Uart(USART_TypeDef *instance, uint32_t baud);

    void send(const uint8_t *data, uint16_t len);

    void send(const char *str);

    /**
     * @return uart状态码
     */
    HAL_StatusTypeDef receive(uint8_t *data, uint16_t len, uint32_t timeout);

    UART_HandleTypeDef *handlePtr();

private:
    UART_HandleTypeDef handle{};
};
