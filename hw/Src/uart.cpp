//
// Created by dingrui on 8/25/26.
//
#include <string.h>

#include "uart.h"

#include "stm32f1xx_hal.h"
#include "stm32f1xx_it.h"

Uart* Uart::registry_[Uart::kMaxUarts] = {};
uint8_t Uart::registryCount_ = 0;

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

Uart::Uart(USART_TypeDef* instance, uint32_t baud) {
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

Uart::~Uart() {
    // 注销
    for (uint8_t i = 0; i < registryCount_; i++) {
        if (registry_[i] == this) {
            registry_[i] = registry_[registryCount_ - 1];
            registryCount_--;
            break;
        }
    }
}

void Uart::send(const uint8_t* data, uint16_t len) {
    HAL_UART_Transmit(&handle, const_cast<uint8_t*>(data), len, HAL_MAX_DELAY);
}

void Uart::send(const char* str) {
    send(reinterpret_cast<const uint8_t*>(str), static_cast<uint16_t>(strlen(str)));
}

HAL_StatusTypeDef Uart::receive(uint8_t* data, uint16_t len, uint32_t timeout) {
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

void Uart::startRx() {
    armRx();
}

void Uart::armRx() {
    HAL_UART_Receive_IT(&handle, &rxByte_, 1);
}

uint16_t Uart::available() const {
    return rxBuf_.available();
}

bool Uart::read(uint8_t& byte) {
    return rxBuf_.read(byte);
}

void Uart::clearRx() {
    rxBuf_.clear();
}

HAL_StatusTypeDef Uart::sendIt(const uint8_t* data, uint16_t len) {
    return HAL_UART_Transmit_IT(&handle, const_cast<uint8_t*>(data), len);
}

void Uart::abortReceive() {
    HAL_UART_AbortReceive(&handle);
}

void Uart::onRxByte(uint8_t byte) {
    rxBuf_.write(byte);
}

void Uart::onRxCplt() {
    // 中断接收到的1字节先缓存起来
    onRxByte(rxByte_);
    // 中断接收是一次性的 重新使能
    armRx();
}

void Uart::onRxError() {
    // 溢出等错误会使接收停止 清标志后重新使能
    __HAL_UART_CLEAR_OREFLAG(&handle);
    armRx();
}

Uart* Uart::fromInstance(USART_TypeDef* instance) {
    for (uint8_t i = 0; i < registryCount_; i++) {
        if (registry_[i]->handle.Instance == instance) {
            return registry_[i];
        }
    }
    return nullptr;
}

Uart* Uart::fromHandle(UART_HandleTypeDef* huart) {
    for (uint8_t i = 0; i < registryCount_; i++) {
        if (&registry_[i]->handle == huart) {
            return registry_[i];
        }
    }
    return nullptr;
}

// 中断服务程序 分发到对应Uart
void USART1_IRQHandler(void) {
    Uart* u = Uart::fromInstance(USART1);
    if (u != nullptr) {
        HAL_UART_IRQHandler(u->handlePtr());
    }
}

void USART2_IRQHandler(void) {
    Uart* u = Uart::fromInstance(USART2);
    if (u != nullptr) {
        HAL_UART_IRQHandler(u->handlePtr());
    }
}

void USART3_IRQHandler(void) {
    Uart* u = Uart::fromInstance(USART3);
    if (u != nullptr) {
        HAL_UART_IRQHandler(u->handlePtr());
    }
}

// 中断收满回调 分发到对应Uart
void HAL_UART_RxCpltCallback(UART_HandleTypeDef* huart) {
    Uart* u = Uart::fromHandle(huart);
    if (u != nullptr) {
        u->onRxCplt();
    }
}

// 中断出错回调 分发到对应Uart
void HAL_UART_ErrorCallback(UART_HandleTypeDef* huart) {
    Uart* u = Uart::fromHandle(huart);
    if (u != nullptr) {
        u->onRxError();
    }
}