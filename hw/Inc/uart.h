//
// Created by dingrui on 8/25/26.
//

#pragma once

#include <stdint.h>

#include <stm32f1xx_hal.h>

#include "ring_buffer.h"

// 8n1
class Uart {
public:
    Uart(USART_TypeDef* instance, uint32_t baud);

    ~Uart();

    // 阻塞收发
    void send(const uint8_t* data, uint16_t len);

    void send(const char* str);

    HAL_StatusTypeDef receive(uint8_t* data, uint16_t len, uint32_t timeout);

    // 中断接收 启动后收到的字节自动写入环形缓冲
    void startRx();

    // 环形缓冲中可读的字节数
    uint16_t available() const;

    // 从环形缓冲读出一个字节 空返回false
    bool read(uint8_t& byte);

    // 清空环形缓冲
    void clearRx();

    // 中断发送
    HAL_StatusTypeDef sendIt(const uint8_t* data, uint16_t len);

    // 中止中断接收
    void abortReceive();

    UART_HandleTypeDef* handlePtr() {
        return &handle;
    }

    // 收到一个字节后被调用 默认写入环形缓冲 派生类可覆写
    virtual void onRxByte(uint8_t byte);

    // 中断完成/出错处理 供HAL回调分发
    void onRxCplt();

    void onRxError();

    // 供ISR和HAL回调反查对象
    static Uart* fromInstance(USART_TypeDef* instance);

    static Uart* fromHandle(UART_HandleTypeDef* huart);

private:
    // 使能单字节中断接收 为什么1个字节就触发1次中断 因为发送来的数据不固定大小 没法定下来中断接收的那个size 所以就定1 只要有数据过来就收 然后我自己缓存起来
    void armRx();

    UART_HandleTypeDef handle{};
    // 中断接收时候指定了size是1 meici每次收到的数据就先放在这
    uint8_t rxByte_ = 0;
    // 中断接收到的数据先缓冲在这 等到发送时机的时候把这个地方缓存着的逐个字节发送出去
    RingBuffer rxBuf_;

    static constexpr uint8_t kMaxUarts = 3;
    static Uart* registry_[kMaxUarts];
    static uint8_t registryCount_;
};