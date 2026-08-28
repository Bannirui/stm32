//
// Created by dingrui on 8/27/26.
//

#pragma once

#include <stdint.h>

#include <stm32f1xx_hal.h>

// 定时器基类 统一:时钟使能/时基配置/注册表/中断分发
class Timer {
public:
    /**
     * psc和arr能够确定定时的时间
     * @param instance TIM1/2/3/4
     * @param psc 预分频 (计数频率 = 内核时钟/(psc+1))
     * @param arr 自动重载值 (计满arr触发更新事件)
     */
    Timer(TIM_TypeDef* instance, uint16_t psc, uint16_t arr);

    virtual ~Timer();

    // 启动/停止计数 虚拟 派生类覆写为各自模式的启动
    virtual void start();

    virtual void stop();

    // 当前计数值
    uint32_t count() const;

    // 虚回调 中断时由HAL回调分发过来 用户可覆写
    virtual void onPeriodElapsed() {
    }

    // 输入捕获到新值 用户可覆写
    virtual void onCapture(uint32_t val) {
    }

    // 中断服务入口 调用HAL_TIM_IRQHandler 由具体定时器的IRQHandler调用
    virtual void handleIrq();

    // ISR与HAL回调里反查定时器对象
    static Timer* fromInstance(TIM_TypeDef* instance);

    static Timer* fromHandle(TIM_HandleTypeDef* htim);

protected:
    // 定时器内核时钟 APB分频>1时等于总线时钟×2
    uint32_t kernelClock() const;

    // 定时器总控结构体
    TIM_HandleTypeDef handle{};

    // F103中容量(STM32F103xB)只有TIM1~TIM4 无TIM5/6/7
    static constexpr uint8_t kMaxTimers = 4;
    // 缓存各定时器对象 中断回调的时候要找到是哪个定时器 TIM1/2/3/4
    static Timer* registry_[kMaxTimers];
    static uint8_t registryCount_;
};

// 周期定时中断 溢出时递增计数 可选用户回调
class TimerTick : public Timer {
public:
    /**
     * @param hz 每秒中断次数 内部自动算好psc/arr
     */
    TimerTick(TIM_TypeDef* instance, uint32_t hz);

    void start() override;

    void stop() override;

    // 每次溢出调用的用户回调
    void setCallback(void (*cb)());

    // 从启动以来的运行时长 (hz能整除1000/1000000时精确)
    uint64_t millis() const;

    uint64_t micros() const;

    void onPeriodElapsed() override;

private:
    // 1s时间hz次 1次就是1/hz秒
    uint32_t hz_;
    // tick次 1次=1/hz秒 然后转到ms和ns
    volatile uint32_t tick_ = 0;
    // 溢出时回调用户的回调函数
    void (*cb_)() = nullptr;
};

// PWM输出 需有通道的定时器 TIM2/3/4
class TimerPwm : public Timer {
public:
    /**
     * @param channel TIM_CHANNEL_1~4 引脚按默认映射自动配置
     * @param psc 预分频
     * @param arr 自动重载值 决定频率 频率 = 内核时钟/((psc+1)*(arr+1))
     */
    TimerPwm(TIM_TypeDef* instance, uint32_t channel, uint16_t psc, uint16_t arr);

    void start() override;

    void stop() override;

    // 占空比 0~100
    void setDuty(uint8_t percent);

    // 直接写比较值CCR
    void setCompare(uint16_t ccr);

    // 改输出频率 保持当前占空比
    void setFrequency(uint32_t hz);

private:
    uint32_t channel_;
};

// 高级定时器PWM TIM1专用: 互补输出+死区+刹车+重复计数
class TimerPwmAdv : public TimerPwm {
public:
    struct Config {
        uint16_t psc = 0;                    // 预分频
        uint16_t arr = 0xFFFF;               // 自动重载值 频率 = 内核时钟/((psc+1)*(arr+1))
        uint32_t channel = TIM_CHANNEL_1;    // TIM_CHANNEL_1~3 (CH4没有互补N)
        bool complement = false;             // 使能互补输出 CH1N/2N/3N = PB13/14/15
        uint8_t deadTime = 0;                // 死区 DTG 0~255
        bool breakEnable = false;            // 使能刹车输入 PA12
        uint32_t breakPolarity = TIM_BREAKPOLARITY_LOW;
        bool automaticOutput = false;        // 刹车解除后自动恢复输出 AOE
        bool offStateHighZ = false;          // MOE=0时输出高阻 否则输出非激活电平
        uint32_t lockLevel = TIM_LOCKLEVEL_OFF;  // 锁定级别 防止运行中被改配置
        uint8_t repetition = 0;              // 重复计数RCR 0=每个周期更新事件
    };

    TimerPwmAdv(TIM_TypeDef* instance, const Config& cfg);

    void start() override;

    void stop() override;

    // 运行中改死区 直接写DTG立即生效
    void setDeadTime(uint8_t dt);

    // 运行中改重复计数 下次更新事件生效
    void setRepetition(uint8_t rcr);

private:
    Config cfg_;
};

// 输入捕获 测频率/脉宽 TIM2/3/4
class TimerCapture : public Timer {
public:
    /**
     * @param channel TIM_CHANNEL_1~4 引脚按默认映射自动配置
     * @param psc 预分频 计数频率 = 内核时钟/(psc+1) 默认不预分频
     */
    TimerCapture(TIM_TypeDef* instance, uint32_t channel, uint16_t psc = 0);

    void start() override;

    // 最近一个上升沿间隔(计数个数) 还没捕获到返回0
    uint32_t lastPeriod() const;

    // 输入信号频率 未捕获返回0
    float frequencyHz() const;

    void onCapture(uint32_t val) override;

private:
    uint32_t channel_;
    uint16_t psc_;
    uint32_t lastCapture_ = 0;
    uint32_t period_ = 0;
    bool haveCapture_ = false;
};

// 编码器接口模式 TIM2/3/4
class TimerEncoder : public Timer {
public:
    TimerEncoder(TIM_TypeDef* instance);

    void start() override;

    // 当前位置 带符号
    int16_t position() const;

    void setPosition(int16_t pos);
};
