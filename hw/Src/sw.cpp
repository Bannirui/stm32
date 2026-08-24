//
// Created by dingrui on 8/23/26.
//

#include "sw.h"

#include "stm32f1xx_hal.h"

// 全局实例
// PC13 扫描模式
SwScan sw8_scan(GPIOC, GPIO_PIN_13);
// PC13 中断模式
SwIt sw8_it(GPIOC, GPIO_PIN_13);
// PA0  中断模式
SwIt sw11_it(GPIOA, GPIO_PIN_0);

// 开时钟
static void enable_gpio_clock(GPIO_TypeDef *port) {
    if (port == GPIOA) {
        __HAL_RCC_GPIOA_CLK_ENABLE();
    } else if (port == GPIOB) {
        __HAL_RCC_GPIOB_CLK_ENABLE();
    } else if (port == GPIOC) {
        __HAL_RCC_GPIOC_CLK_ENABLE();
    } else if (port == GPIOD) {
        __HAL_RCC_GPIOD_CLK_ENABLE();
    } else if (port == GPIOE) {
        __HAL_RCC_GPIOE_CLK_ENABLE();
    }
}

Sw::Sw(GPIO_TypeDef *port, uint16_t pin)
    : port(port), pin(pin) {
    // 开时钟
    enable_gpio_clock(port);
}

bool Sw::read() const {
    return HAL_GPIO_ReadPin(port, pin) == GPIO_PIN_SET;
}

void Sw::initInput(uint32_t pull) {
    GPIO_InitTypeDef init{};
    init.Pin = pin;
    // 输入
    init.Mode = GPIO_MODE_INPUT;
    // 上/下拉
    init.Pull = pull;
    HAL_GPIO_Init(port, &init);
}

SwScan::SwScan(GPIO_TypeDef *port, uint16_t pin)
    : Sw(port, pin) {
}

SwScan::Result SwScan::scan(Edge mode) {
    if (read() && !lastState) {
        // 检测到按键被按下 原来按键是抬起的并且现在被按下
        // 延时消抖
        for (uint32_t i = 0; i < kDebounceTicks; i++) {
            if (!read()) {
                return Result::None;
            }
            // 稳定保持在高电平 说明按键被按下了
            lastState = true;
            if (mode == Edge::Press) {
                return Result::Pressed;
            }
        }
    } else if (!read() && lastState) {
        // 检测按键被抬起 原来的按键是按下的 现在被抬起
        // 延时消抖
        for (uint32_t i = 0; i < kDebounceTicks; i++) {
            if (read()) {
                return Result::None;
            }
            // 稳定保持在低电平 说明按键被抬起了
            lastState = false;
            if (mode == Edge::Release) {
                return Result::Released;
            }
        }
    }
    return Result::None;
}

SwIt::SwIt(GPIO_TypeDef *port, uint16_t pin)
    : Sw(port, pin) {
}

void SwIt::initIt(Edge mode, uint32_t pull, uint8_t priority) {
    GPIO_InitTypeDef init{};
    init.Pin = pin;
    if (mode == Edge::Press) {
        // 按下执行 上升沿
        init.Mode = GPIO_MODE_IT_RISING;
    } else {
        // 抬起执行 下降沿
        init.Mode = GPIO_MODE_IT_FALLING;
    }
    init.Pull = pull;
    HAL_GPIO_Init(port, &init);
    // pin号推断中断线
    IRQn_Type irqn;
    if (pin < GPIO_PIN_5) {
        // EXTI0~4
        irqn = EXTI0_IRQn;
    } else if (pin < GPIO_PIN_10) {
        irqn = EXTI9_5_IRQn;
    } else {
        irqn = EXTI15_10_IRQn;
    }
    // 中断优先级
    HAL_NVIC_SetPriority(irqn, priority, 0);
    // 使能
    HAL_NVIC_EnableIRQ(irqn);
}

void SwIt::handleExti(uint16_t pin) {
    if (pin != this->pin) { return; }
    // 消抖
    bool level = read();
    for (uint32_t i = 0; i < kDebounceTicks; i++) {
        // 不稳定 视为干扰
        if (read() != level) { return; }
    }
    HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_0);
}

// 中断的分发入口
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    sw8_it.handleExti(GPIO_Pin);
    sw11_it.handleExti(GPIO_Pin);
}
