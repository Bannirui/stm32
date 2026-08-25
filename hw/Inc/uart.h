//
// Created by dingrui on 8/25/26.
//

#pragma once

#include <stdint.h>

#include "stm32f1xx_hal.h"

// 8n1
class Uart {
public:
    Uart(USART_TypeDef *instance, uint32_t baud);

    ~Uart();

    void send(const uint8_t *data, uint16_t len);

    void send(const char *str);

    HAL_StatusTypeDef receive(uint8_t *data, uint16_t len, uint32_t timeout);

    // 中断接收len字节 收满后回调onRxCplt
    HAL_StatusTypeDef receiveIt(uint8_t *data, uint16_t len);

    // 中断发送len字节
    HAL_StatusTypeDef sendIt(const uint8_t *data, uint16_t len);

    // 中止中断接收
    void abortReceive();

    // 本次中断收到的数据
    uint8_t *rxBuffer() const { return rxBuf_; }

    uint16_t rxLen() const { return rxLen_; }

    UART_HandleTypeDef *handlePtr() { return &handle; }

    // 中断收满len字节后被调用 派生类覆写实现业务
    virtual void onRxCplt();

    // 供ISR和HAL回调通过实例/句柄反查对象
    static Uart *fromInstance(USART_TypeDef *instance);

    static Uart *fromHandle(UART_HandleTypeDef *huart);

private:
    UART_HandleTypeDef handle{};
    // uart的收到的数据 收到了多少数据
    uint8_t *rxBuf_ = nullptr;
    uint16_t rxLen_ = 0;

    static constexpr uint8_t kMaxUarts = 3;
    static Uart *registry_[kMaxUarts];
    static uint8_t registryCount_;
};
