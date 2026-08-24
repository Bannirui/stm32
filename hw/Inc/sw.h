//
// Created by dingrui on 8/23/26.
//

#pragma once

#include <stdint.h>

#include "stm32f103xb.h"

// 按键
class Sw {
public:
    // 触发方式
    enum class Edge:uint8_t {
        // 按下触发
        Press = 0,
        // 抬起触发
        Release = 1,
    };

public:
    /**
     * @param pull 下拉模式
     */
    void initInput(uint32_t pull);

protected:
    Sw(GPIO_TypeDef *port, uint16_t pin);

    virtual ~Sw() = default;

    /**
     * 读取当前电平
     * @return true表示高电平 false表示低电平
     */
    bool read() const;

protected:
    // 消抖用
    static constexpr uint32_t kDebounceTicks = 0x7fff;
    GPIO_TypeDef *port;
    uint16_t pin;
};

// 扫描模式 轮询+消抖
class SwScan : public Sw {
public:
    // 检测按键执行的动作
    enum class Result: uint8_t {
        // 无动作
        None = 0,
        // 检测到按下
        Pressed = 1,
        // 检测到抬起
        Released = 1 << 1,
    };

    SwScan(GPIO_TypeDef *port, uint16_t pin);

    Result scan(Edge mode);

private:
    // 上次的电平 true表示高电平 false表示低电平
    bool lastState = false;
};

// 中断模式
class SwIt : public Sw {
public:
    SwIt(GPIO_TypeDef *port, uint16_t pin);

    /**
     * @param mode 检测上升沿还是下降沿
     * @param pull 输入模式
     * @param priority 中断优先级
     */
    void initIt(Edge mode, uint32_t pull, uint8_t priority);

    void handleExti(uint16_t pin);
};

// 全局按键实例 定义在sw.cpp
// PC13 扫描模式
extern SwScan sw8_scan;
// PC13 中断模式
extern SwIt sw8_it;
// PA0  中断模式
extern SwIt sw11_it;
