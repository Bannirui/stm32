//
// Created by dingrui on 8/25/26.
//

#pragma once

#include <stdint.h>

#include <stm32f1xx_hal.h>

#include "ring_buffer.h"

// 轮询UART 阻塞收发 可直接使用
class UartBase {
public:
    UartBase(USART_TypeDef* instance, uint32_t baud);

    virtual ~UartBase();

    // 阻塞收发
    void send(const uint8_t* data, uint16_t len);

    void send(const char* str);

    HAL_StatusTypeDef receive(uint8_t* data, uint16_t len, uint32_t timeout);

    // 中断发送
    HAL_StatusTypeDef sendIt(const uint8_t* data, uint16_t len);

    // 中止接收
    void abortReceive();

    UART_HandleTypeDef* handlePtr() {
        return &handle;
    }

    // DMA句柄 仅UartDma有效 供DMA中断分发
    virtual DMA_HandleTypeDef* dmaRxHandle() {
        return nullptr;
    }

    virtual DMA_HandleTypeDef* dmaTxHandle() {
        return nullptr;
    }

    // 虚回调 派生类可覆写
    virtual void onRxByte(uint8_t byte) {
    }

    virtual void onRxIdle() {
    }

    virtual void onTxCplt() {
    }

    // ISR与HAL回调入口 按模式覆写
    virtual void handleIrq();

    virtual void onRxCplt() {
    }

    virtual void onRxError() {
    }

    static UartBase* fromInstance(USART_TypeDef* instance);

    static UartBase* fromHandle(UART_HandleTypeDef* huart);

protected:
    UART_HandleTypeDef handle{};

    static constexpr uint8_t kMaxUarts = 3;
    // 缓存uart1/2/3 中断回调的时候要找到是哪个串口
    static UartBase* registry_[kMaxUarts];
    // 有几种uart串口
    static uint8_t registryCount_;
};

// 带环形缓冲的UART 中断/DMA的公共基类
class UartBuffered : public UartBase {
protected:
    UartBuffered(USART_TypeDef* instance, uint32_t baud)
        : UartBase(instance, baud) {
    }

public:
    ~UartBuffered() override = default;

    // 环形缓冲中可读的字节数
    uint16_t available() const;

    // 从环形缓冲读出一个字节 空返回false
    bool read(uint8_t& byte);

    // 清空环形缓冲
    void clearRx();

protected:
    RingBuffer rxBuf_;
};

// 逐字节中断接收
class UartInterrupt : public UartBuffered {
public:
    UartInterrupt(USART_TypeDef* instance, uint32_t baud)
        : UartBuffered(instance, baud) {
    }

    ~UartInterrupt() override = default;

    // 启动中断接收 字节自动写入环形缓冲
    void startRx();

    // 停止中断接收
    void stopRx();

    // 收到一字节回调 写入环形缓冲并重新使能
    void onRxCplt() override;

    // 出错后重新使能
    void onRxError() override;

private:
    void armRx();

    uint8_t rxByte_ = 0;
};

// DMA环形+空闲中断接收
class UartDma : public UartBuffered {
public:
    UartDma(USART_TypeDef* instance, uint32_t baud);

    ~UartDma() override = default;

    // DMA发送 一次性
    HAL_StatusTypeDef sendDma(const uint8_t* data, uint16_t len);

    // DMA接收 环形+空闲中断 数据写入环形缓冲
    HAL_StatusTypeDef startRxDma(uint8_t* buf, uint16_t bufSize);

    // 停止DMA接收
    void stopRxDma();

    DMA_HandleTypeDef* dmaRxHandle() override;

    DMA_HandleTypeDef* dmaTxHandle() override;

    // ISR入口 空闲中断收帧
    void handleIrq() override;

    // 溢出后重启DMA
    void onRxError() override;

private:
    void setupDma();

    DMA_HandleTypeDef hdmaRx_{};
    DMA_HandleTypeDef hdmaTx_{};
    uint8_t* dmaRxBuf_ = nullptr;
    uint16_t dmaRxBufSize_ = 0;
    uint16_t dmaRxPos_ = 0;
    uint16_t prevCounter_ = 0;
};