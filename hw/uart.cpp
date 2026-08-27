//
// Created by dingrui on 8/25/26.
//
#include <string.h>

#include "uart.h"

#include <stm32f1xx_hal.h>
#include <stm32f1xx_it.h>

UartBase* UartBase::registry_[UartBase::kMaxUarts] = {};
uint8_t UartBase::registryCount_ = 0;

// 串口引脚、时钟、中断 在MspInit中按实例分发
void HAL_UART_MspInit(UART_HandleTypeDef* huart) {
    GPIO_InitTypeDef init{};
    if (huart->Instance == USART1) {
        __HAL_RCC_USART1_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();
        init.Pin = GPIO_PIN_9 | GPIO_PIN_10; // PA9 TX  PA10 RX
        init.Mode = GPIO_MODE_AF_PP;
        init.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(GPIOA, &init);
        HAL_NVIC_SetPriority(USART1_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(USART1_IRQn);
    } else if (huart->Instance == USART2) {
        __HAL_RCC_USART2_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();
        init.Pin = GPIO_PIN_2 | GPIO_PIN_3; // PA2 TX  PA3 RX
        init.Mode = GPIO_MODE_AF_PP;
        init.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(GPIOA, &init);
        HAL_NVIC_SetPriority(USART2_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(USART2_IRQn);
    } else if (huart->Instance == USART3) {
        __HAL_RCC_USART3_CLK_ENABLE();
        __HAL_RCC_GPIOB_CLK_ENABLE();
        init.Pin = GPIO_PIN_10 | GPIO_PIN_11; // PB10 TX  PB11 RX
        init.Mode = GPIO_MODE_AF_PP;
        init.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(GPIOB, &init);
        HAL_NVIC_SetPriority(USART3_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(USART3_IRQn);
    }
}

UartBase::UartBase(USART_TypeDef* instance, uint32_t baud) {
    handle.Instance = instance;
    handle.Init.BaudRate = baud;
    handle.Init.WordLength = UART_WORDLENGTH_8B;
    handle.Init.StopBits = UART_STOPBITS_1;
    handle.Init.Parity = UART_PARITY_NONE;
    handle.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    handle.Init.Mode = UART_MODE_TX_RX;
    HAL_UART_Init(&handle);
    // 注册 供ISR和HAL回调反查对象
    if (registryCount_ < kMaxUarts) {
        registry_[registryCount_++] = this;
    }
}

UartBase::~UartBase() {
    // 注销
    for (uint8_t i = 0; i < registryCount_; i++) {
        if (registry_[i] == this) {
            registry_[i] = registry_[registryCount_ - 1];
            registryCount_--;
            break;
        }
    }
}

void UartBase::send(const uint8_t* data, uint16_t len) {
    HAL_UART_Transmit(&handle, const_cast<uint8_t*>(data), len, HAL_MAX_DELAY);
}

void UartBase::send(const char* str) {
    send(reinterpret_cast<const uint8_t*>(str), static_cast<uint16_t>(strlen(str)));
}

HAL_StatusTypeDef UartBase::receive(uint8_t* data, uint16_t len, uint32_t timeout) {
    HAL_StatusTypeDef status = HAL_UART_Receive(&handle, data, len, timeout);
    if (status == HAL_TIMEOUT) {
        // 超时返回后HAL不会复位状态也不会解锁 会导致下一次调用直接返回HAL_BUSY
        handle.gState = HAL_UART_STATE_READY;
        handle.RxState = HAL_UART_STATE_READY;
        handle.Lock = HAL_UNLOCKED;
        handle.ErrorCode = HAL_UART_ERROR_NONE;
    }
    return status;
}

HAL_StatusTypeDef UartBase::sendIt(const uint8_t* data, uint16_t len) {
    return HAL_UART_Transmit_IT(&handle, const_cast<uint8_t*>(data), len);
}

void UartBase::abortReceive() {
    HAL_UART_AbortReceive(&handle);
}

void UartBase::handleIrq() {
    HAL_UART_IRQHandler(&handle);
}

UartBase* UartBase::fromInstance(USART_TypeDef* instance) {
    for (uint8_t i = 0; i < registryCount_; i++) {
        if (registry_[i]->handle.Instance == instance) {
            return registry_[i];
        }
    }
    return nullptr;
}

UartBase* UartBase::fromHandle(UART_HandleTypeDef* huart) {
    for (uint8_t i = 0; i < registryCount_; i++) {
        if (&registry_[i]->handle == huart) {
            return registry_[i];
        }
    }
    return nullptr;
}

uint16_t UartBuffered::available() const {
    return rxBuf_.available();
}

bool UartBuffered::read(uint8_t& byte) {
    return rxBuf_.read(byte);
}

void UartBuffered::clearRx() {
    rxBuf_.clear();
}

void UartInterrupt::startRx() {
    HAL_UART_AbortReceive(&handle);
    __HAL_UART_DISABLE_IT(&handle, UART_IT_IDLE);
    armRx();
}

void UartInterrupt::stopRx() {
    HAL_UART_AbortReceive(&handle);
    __HAL_UART_DISABLE_IT(&handle, UART_IT_IDLE);
}

void UartInterrupt::armRx() {
    HAL_UART_Receive_IT(&handle, &rxByte_, 1);
}

void UartInterrupt::onRxCplt() {
    rxBuf_.write(rxByte_);
    onRxByte(rxByte_);
    // 中断接收是一次性的 重新使能
    armRx();
}

void UartInterrupt::onRxError() {
    __HAL_UART_CLEAR_OREFLAG(&handle);
    armRx();
}

UartDma::UartDma(USART_TypeDef* instance, uint32_t baud)
    : UartBuffered(instance, baud) {
    setupDma();
}

void UartDma::setupDma() {
    DMA_Channel_TypeDef* rxCh = nullptr;
    DMA_Channel_TypeDef* txCh = nullptr;
    IRQn_Type rxIrq = (IRQn_Type)0;
    IRQn_Type txIrq = (IRQn_Type)0;
    if (handle.Instance == USART1) {
        rxCh = DMA1_Channel5;
        txCh = DMA1_Channel4;
        rxIrq = DMA1_Channel5_IRQn;
        txIrq = DMA1_Channel4_IRQn;
    } else if (handle.Instance == USART2) {
        rxCh = DMA1_Channel6;
        txCh = DMA1_Channel7;
        rxIrq = DMA1_Channel6_IRQn;
        txIrq = DMA1_Channel7_IRQn;
    } else if (handle.Instance == USART3) {
        rxCh = DMA1_Channel2;
        txCh = DMA1_Channel3;
        rxIrq = DMA1_Channel2_IRQn;
        txIrq = DMA1_Channel3_IRQn;
    }
    __HAL_RCC_DMA1_CLK_ENABLE();

    // RX DMA 环形 外设->内存 不定长接收
    hdmaRx_.Instance = rxCh;
    hdmaRx_.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdmaRx_.Init.PeriphInc = DMA_PINC_DISABLE;
    hdmaRx_.Init.MemInc = DMA_MINC_ENABLE;
    hdmaRx_.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdmaRx_.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdmaRx_.Init.Mode = DMA_CIRCULAR;
    hdmaRx_.Init.Priority = DMA_PRIORITY_HIGH;
    HAL_DMA_Init(&hdmaRx_);
    __HAL_LINKDMA(&handle, hdmarx, hdmaRx_);
    HAL_NVIC_SetPriority(rxIrq, 0, 0);
    HAL_NVIC_EnableIRQ(rxIrq);

    // TX DMA 普通 内存->外设 一次性发送
    hdmaTx_.Instance = txCh;
    hdmaTx_.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdmaTx_.Init.PeriphInc = DMA_PINC_DISABLE;
    hdmaTx_.Init.MemInc = DMA_MINC_ENABLE;
    hdmaTx_.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdmaTx_.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdmaTx_.Init.Mode = DMA_NORMAL;
    hdmaTx_.Init.Priority = DMA_PRIORITY_HIGH;
    HAL_DMA_Init(&hdmaTx_);
    __HAL_LINKDMA(&handle, hdmatx, hdmaTx_);
    HAL_NVIC_SetPriority(txIrq, 0, 0);
    HAL_NVIC_EnableIRQ(txIrq);
}

HAL_StatusTypeDef UartDma::sendDma(const uint8_t* data, uint16_t len) {
    return HAL_UART_Transmit_DMA(&handle, const_cast<uint8_t*>(data), len);
}

HAL_StatusTypeDef UartDma::startRxDma(uint8_t* buf, uint16_t bufSize) {
    HAL_UART_AbortReceive(&handle);
    __HAL_UART_DISABLE_IT(&handle, UART_IT_IDLE);
    dmaRxBuf_ = buf;
    dmaRxBufSize_ = bufSize;
    dmaRxPos_ = 0;
    prevCounter_ = bufSize;
    HAL_StatusTypeDef status = HAL_UART_Receive_DMA(&handle, buf, bufSize);
    if (status == HAL_OK) {
        // 线空闲时触发中断 判断一帧收完
        __HAL_UART_ENABLE_IT(&handle, UART_IT_IDLE);
    }
    return status;
}

void UartDma::stopRxDma() {
    HAL_UART_AbortReceive(&handle);
    __HAL_UART_DISABLE_IT(&handle, UART_IT_IDLE);
}

DMA_HandleTypeDef* UartDma::dmaRxHandle() {
    return &hdmaRx_;
}

DMA_HandleTypeDef* UartDma::dmaTxHandle() {
    return &hdmaTx_;
}

void UartDma::handleIrq() {
    // 线空闲=一帧收完 把DMA缓冲新到的数据转进环形缓冲
    if (__HAL_UART_GET_IT_SOURCE(&handle, UART_IT_IDLE) &&
        __HAL_UART_GET_FLAG(&handle, UART_FLAG_IDLE)) {
        __HAL_UART_CLEAR_IDLEFLAG(&handle);
        uint16_t counter = __HAL_DMA_GET_COUNTER(&hdmaRx_);
        uint16_t received = (prevCounter_ + dmaRxBufSize_ - counter) % dmaRxBufSize_;
        for (uint16_t i = 0; i < received; i++) {
            rxBuf_.write(dmaRxBuf_[dmaRxPos_]);
            dmaRxPos_ = (dmaRxPos_ + 1) % dmaRxBufSize_;
        }
        prevCounter_ = counter;
        onRxIdle();
    }
    HAL_UART_IRQHandler(&handle);
}

void UartDma::onRxError() {
    __HAL_UART_CLEAR_OREFLAG(&handle);
    // 溢出使DMA接收停止 重新启动 位置计数同步复位
    dmaRxPos_ = 0;
    prevCounter_ = dmaRxBufSize_;
    HAL_UART_Receive_DMA(&handle, dmaRxBuf_, dmaRxBufSize_);
    __HAL_UART_ENABLE_IT(&handle, UART_IT_IDLE);
}

// 串口中断服务程序 分发到对应Uart
void USART1_IRQHandler(void) {
    UartBase* u = UartBase::fromInstance(USART1);
    if (u != nullptr) {
        u->handleIrq();
    }
}

void USART2_IRQHandler(void) {
    UartBase* u = UartBase::fromInstance(USART2);
    if (u != nullptr) {
        u->handleIrq();
    }
}

void USART3_IRQHandler(void) {
    UartBase* u = UartBase::fromInstance(USART3);
    if (u != nullptr) {
        u->handleIrq();
    }
}

// DMA1中断服务程序 USART3_TX=Ch2 USART3_RX=Ch3 USART1_TX=Ch4 USART1_RX=Ch5 USART2_RX=Ch6 USART2_TX=Ch7
void DMA1_Channel2_IRQHandler(void) {
    UartBase* u = UartBase::fromInstance(USART3);
    if (u != nullptr) {
        DMA_HandleTypeDef* hdma = u->dmaTxHandle();
        if (hdma != nullptr) {
            HAL_DMA_IRQHandler(hdma);
        }
    }
}

void DMA1_Channel3_IRQHandler(void) {
    UartBase* u = UartBase::fromInstance(USART3);
    if (u != nullptr) {
        DMA_HandleTypeDef* hdma = u->dmaRxHandle();
        if (hdma != nullptr) {
            HAL_DMA_IRQHandler(hdma);
        }
    }
}

void DMA1_Channel4_IRQHandler(void) {
    UartBase* u = UartBase::fromInstance(USART1);
    if (u != nullptr) {
        DMA_HandleTypeDef* hdma = u->dmaTxHandle();
        if (hdma != nullptr) {
            HAL_DMA_IRQHandler(hdma);
        }
    }
}

void DMA1_Channel5_IRQHandler(void) {
    UartBase* u = UartBase::fromInstance(USART1);
    if (u != nullptr) {
        DMA_HandleTypeDef* hdma = u->dmaRxHandle();
        if (hdma != nullptr) {
            HAL_DMA_IRQHandler(hdma);
        }
    }
}

void DMA1_Channel6_IRQHandler(void) {
    UartBase* u = UartBase::fromInstance(USART2);
    if (u != nullptr) {
        DMA_HandleTypeDef* hdma = u->dmaRxHandle();
        if (hdma != nullptr) {
            HAL_DMA_IRQHandler(hdma);
        }
    }
}

void DMA1_Channel7_IRQHandler(void) {
    UartBase* u = UartBase::fromInstance(USART2);
    if (u != nullptr) {
        DMA_HandleTypeDef* hdma = u->dmaTxHandle();
        if (hdma != nullptr) {
            HAL_DMA_IRQHandler(hdma);
        }
    }
}

// 中断收满回调 分发到对应Uart
void HAL_UART_RxCpltCallback(UART_HandleTypeDef* huart) {
    UartBase* u = UartBase::fromHandle(huart);
    if (u != nullptr) {
        u->onRxCplt();
    }
}

// 中断发送完成回调 分发到对应Uart
void HAL_UART_TxCpltCallback(UART_HandleTypeDef* huart) {
    UartBase* u = UartBase::fromHandle(huart);
    if (u != nullptr) {
        u->onTxCplt();
    }
}

// 中断出错回调 分发到对应Uart
void HAL_UART_ErrorCallback(UART_HandleTypeDef* huart) {
    UartBase* u = UartBase::fromHandle(huart);
    if (u != nullptr) {
        u->onRxError();
    }
}