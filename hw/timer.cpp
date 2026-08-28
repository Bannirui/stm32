//
// Created by dingrui on 8/27/26.
//

#include "timer.h"

#include <stm32f1xx_hal.h>

Timer* Timer::registry_[Timer::kMaxTimers] = {};
uint8_t Timer::registryCount_ = 0;

/**
 * 使能定时器时钟
 * @param instance TIM1/2/3/4
 */
static void enableTimerClock(TIM_TypeDef* instance) {
    if (instance == TIM1) {
        __HAL_RCC_TIM1_CLK_ENABLE();
    } else if (instance == TIM2) {
        __HAL_RCC_TIM2_CLK_ENABLE();
    } else if (instance == TIM3) {
        __HAL_RCC_TIM3_CLK_ENABLE();
    } else if (instance == TIM4) {
        __HAL_RCC_TIM4_CLK_ENABLE();
    }
}

// 使能GPIO时钟
static void enableGpioClock(GPIO_TypeDef* port) {
    if (port == GPIOA) {
        __HAL_RCC_GPIOA_CLK_ENABLE();
    } else if (port == GPIOB) {
        __HAL_RCC_GPIOB_CLK_ENABLE();
    } else if (port == GPIOC) {
        __HAL_RCC_GPIOC_CLK_ENABLE();
    } else if (port == GPIOD) {
        __HAL_RCC_GPIOD_CLK_ENABLE();
    }
}

// 通道对应的默认引脚映射
static bool channelPin(TIM_TypeDef* instance, uint32_t channel, GPIO_TypeDef*& port, uint16_t& pin) {
    struct Entry {
        TIM_TypeDef* tim;
        GPIO_TypeDef* port;
        uint16_t pin;
    };
    static const Entry map[] = {
        // TIM1
        {TIM1, GPIOA, GPIO_PIN_8},
        {TIM1, GPIOA, GPIO_PIN_9},
        {TIM1, GPIOA, GPIO_PIN_10},
        {TIM1, GPIOA, GPIO_PIN_11},
        // TIM2
        {TIM2, GPIOA, GPIO_PIN_0},
        {TIM2, GPIOA, GPIO_PIN_1},
        {TIM2, GPIOA, GPIO_PIN_2},
        {TIM2, GPIOA, GPIO_PIN_3},
        // TIM3
        {TIM3, GPIOA, GPIO_PIN_6},
        {TIM3, GPIOA, GPIO_PIN_7},
        {TIM3, GPIOB, GPIO_PIN_0},
        {TIM3, GPIOB, GPIO_PIN_1},
        // TIM4
        {TIM4, GPIOB, GPIO_PIN_6},
        {TIM4, GPIOB, GPIO_PIN_7},
        {TIM4, GPIOB, GPIO_PIN_8},
        {TIM4, GPIOB, GPIO_PIN_9},
    };
    uint32_t base;
    if (instance == TIM1) {
        base = 0;
    } else if (instance == TIM2) {
        base = 4;
    } else if (instance == TIM3) {
        base = 8;
    } else if (instance == TIM4) {
        base = 12;
    } else {
        return false;
    }
    // TIM_CHANNEL_1/2/3/4 = 0/4/8/12
    uint32_t idx = channel >> 2;
    if (idx >= 4) {
        return false;
    }
    const Entry& e = map[base + idx];
    port = e.port;
    pin = e.pin;
    return true;
}

// 配置通道引脚 output=true推挽复用输出 false浮空输入
static void configureChannelGpio(TIM_TypeDef* instance, uint32_t channel, bool output) {
    GPIO_TypeDef* port = nullptr;
    uint16_t pin = 0;
    if (!channelPin(instance, channel, port, pin)) {
        return;
    }
    enableGpioClock(port);
    GPIO_InitTypeDef init{};
    init.Pin = pin;
    if (output) {
        init.Mode = GPIO_MODE_AF_PP;
        init.Speed = GPIO_SPEED_FREQ_HIGH;
    } else {
        init.Mode = GPIO_MODE_INPUT;
        init.Pull = GPIO_NOPULL;
    }
    HAL_GPIO_Init(port, &init);
}

// 由目标频率算时基 满足 (psc+1)*(arr+1) ≈ 内核时钟/hz 分辨率尽量高
static void timebaseForHertz(uint32_t timerClk, uint32_t hz, uint16_t& psc, uint16_t& arr) {
    if (hz == 0) {
        hz = 1;
    }
    uint32_t rel = timerClk / hz;
    for (uint32_t p = 0; p <= 0xFFFF; p++) {
        uint32_t a = rel / (p + 1) - 1;
        if (a <= 0xFFFF) {
            psc = static_cast<uint16_t>(p);
            arr = static_cast<uint16_t>(a);
            return;
        }
    }
    psc = 0xFFFF;
    arr = 0xFFFF;
}

// 定时器中断号 需要CC中断时用TIM1_CC
static IRQn_Type irqFor(TIM_TypeDef* instance, bool cc) {
    if (instance == TIM1) {
        return cc ? TIM1_CC_IRQn : TIM1_UP_IRQn;
    }
    if (instance == TIM2) {
        return TIM2_IRQn;
    }
    if (instance == TIM3) {
        return TIM3_IRQn;
    }
    return TIM4_IRQn;
}

Timer::Timer(TIM_TypeDef* instance, uint16_t psc, uint16_t arr) {
    enableTimerClock(instance);

    handle.Instance = instance;
    // 预分频
    handle.Init.Prescaler = psc;
    // 计数模式 向上计数
    handle.Init.CounterMode = TIM_COUNTERMODE_UP;
    // 自动重载值
    handle.Init.Period = arr;
    // 对基础定时器没有用处 只对高级定时器有用
    handle.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    handle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    // 初始化定时器基础配置
    HAL_TIM_Base_Init(&handle);

    // 注册 供ISR和HAL回调反查对象
    if (registryCount_ < kMaxTimers) {
        registry_[registryCount_++] = this;
    }
}

Timer::~Timer() {
    // 注销
    for (uint8_t i = 0; i < registryCount_; i++) {
        if (registry_[i] == this) {
            registry_[i] = registry_[registryCount_ - 1];
            registryCount_--;
            break;
        }
    }
}

void Timer::start() {
    // 开启定时器
    HAL_TIM_Base_Start(&handle);
}

void Timer::stop() {
    // 关定时器
    HAL_TIM_Base_Stop(&handle);
}

uint32_t Timer::count() const {
    return __HAL_TIM_GET_COUNTER(&handle);
}

void Timer::handleIrq() {
    HAL_TIM_IRQHandler(&handle);
}

Timer* Timer::fromInstance(TIM_TypeDef* instance) {
    for (uint8_t i = 0; i < registryCount_; i++) {
        if (registry_[i]->handle.Instance == instance) {
            return registry_[i];
        }
    }
    return nullptr;
}

Timer* Timer::fromHandle(TIM_HandleTypeDef* htim) {
    for (uint8_t i = 0; i < registryCount_; i++) {
        if (&registry_[i]->handle == htim) {
            return registry_[i];
        }
    }
    return nullptr;
}

uint32_t Timer::kernelClock() const {
    // TIM1 在APB2上 其余在APB1上
    uint32_t pclk;
    uint32_t presc;
    if (handle.Instance == TIM1) {
        pclk = HAL_RCC_GetPCLK2Freq();
        presc = (RCC->CFGR & RCC_CFGR_PPRE2) >> RCC_CFGR_PPRE2_Pos;
    } else {
        pclk = HAL_RCC_GetPCLK1Freq();
        presc = (RCC->CFGR & RCC_CFGR_PPRE1) >> RCC_CFGR_PPRE1_Pos;
    }
    // APB分频>1时 定时器时钟翻倍
    uint32_t timerClk = pclk;
    if (presc != 0) {
        timerClk *= 2;
    }
    return timerClk;
}

TimerTick::TimerTick(TIM_TypeDef* instance, uint32_t hz)
    : Timer(instance, 0, 0), hz_(hz ? hz : 1) {
    uint16_t psc, arr;
    timebaseForHertz(kernelClock(), hz_, psc, arr);
    handle.Init.Prescaler = psc;
    handle.Init.Period = arr;
    HAL_TIM_Base_Init(&handle);

    HAL_NVIC_SetPriority(irqFor(instance, false), 2, 0);
    HAL_NVIC_EnableIRQ(irqFor(instance, false));
}

void TimerTick::start() {
    HAL_TIM_Base_Start_IT(&handle);
}

void TimerTick::stop() {
    HAL_TIM_Base_Stop_IT(&handle);
}

void TimerTick::setCallback(void (*cb)()) {
    cb_ = cb;
}

uint64_t TimerTick::millis() const {
    return static_cast<uint64_t>(tick_) * 1000 / hz_;
}

uint64_t TimerTick::micros() const {
    return static_cast<uint64_t>(tick_) * 1000000 / hz_;
}

void TimerTick::onPeriodElapsed() {
    tick_++;
    if (cb_) {
        cb_();
    }
}

TimerPwm::TimerPwm(TIM_TypeDef* instance, uint32_t channel, uint16_t psc, uint16_t arr)
    : Timer(instance, psc, arr), channel_(channel) {
    configureChannelGpio(instance, channel, true);

    TIM_OC_InitTypeDef oc{};
    oc.OCMode = TIM_OCMODE_PWM1;
    oc.Pulse = 0;
    oc.OCPolarity = TIM_OCPOLARITY_HIGH;
    oc.OCFastMode = TIM_OCFAST_DISABLE;
    HAL_TIM_PWM_ConfigChannel(&handle, &oc, channel);
}

void TimerPwm::start() {
    HAL_TIM_PWM_Start(&handle, channel_);
}

void TimerPwm::stop() {
    HAL_TIM_PWM_Stop(&handle, channel_);
}

void TimerPwm::setDuty(uint8_t percent) {
    if (percent > 100) {
        percent = 100;
    }
    uint16_t arr = handle.Init.Period;
    __HAL_TIM_SET_COMPARE(&handle, channel_, static_cast<uint16_t>((uint32_t)arr * percent / 100));
}

void TimerPwm::setCompare(uint16_t ccr) {
    __HAL_TIM_SET_COMPARE(&handle, channel_, ccr);
}

void TimerPwm::setFrequency(uint32_t hz) {
    uint16_t psc, arr;
    timebaseForHertz(kernelClock(), hz, psc, arr);
    // 重算前按千分比保留当前占空比
    uint32_t oldArr = handle.Init.Period + 1;
    uint32_t ccr = __HAL_TIM_GET_COMPARE(&handle, channel_);
    uint32_t duty = oldArr ? (ccr * 1000 / oldArr) : 0;

    handle.Init.Prescaler = psc;
    handle.Init.Period = arr;
    __HAL_TIM_SET_PRESCALER(&handle, psc);
    __HAL_TIM_SET_AUTORELOAD(&handle, arr);
    __HAL_TIM_SET_COMPARE(&handle, channel_, static_cast<uint16_t>(arr * duty / 1000));
    // 重新生成更新事件 让新配置立即生效
    __HAL_TIM_SET_COUNTER(&handle, 0);
    handle.Instance->EGR = TIM_EGR_UG;
}

// 互补输出引脚 只有TIM1有 CH1N/2N/3N = PB13/14/15
static bool complementaryPin(uint32_t channel, GPIO_TypeDef*& port, uint16_t& pin) {
    if (channel == TIM_CHANNEL_1) {
        port = GPIOB;
        pin = GPIO_PIN_13;
        return true;
    }
    if (channel == TIM_CHANNEL_2) {
        port = GPIOB;
        pin = GPIO_PIN_14;
        return true;
    }
    if (channel == TIM_CHANNEL_3) {
        port = GPIOB;
        pin = GPIO_PIN_15;
        return true;
    }
    return false;  // CH4没有互补输出
}

TimerPwmAdv::TimerPwmAdv(TIM_TypeDef* instance, const Config& cfg)
    : TimerPwm(instance, cfg.channel, cfg.psc, cfg.arr), cfg_(cfg) {
    // 高级定时器功能只存在于TIM1 编译USE_FULL_ASSERT时才会检查
    assert_param(IS_TIM_ADVANCED_INSTANCE(instance));

    // 重复计数 只对TIM1有效 重新初始化让RCR预装载生效
    handle.Init.RepetitionCounter = cfg.repetition;
    HAL_TIM_Base_Init(&handle);

    // 互补输出引脚
    if (cfg.complement) {
        GPIO_TypeDef* port = nullptr;
        uint16_t pin = 0;
        if (complementaryPin(cfg.channel, port, pin)) {
            enableGpioClock(port);
            GPIO_InitTypeDef init{};
            init.Pin = pin;
            init.Mode = GPIO_MODE_AF_PP;
            init.Speed = GPIO_SPEED_FREQ_HIGH;
            HAL_GPIO_Init(port, &init);
        }
    }

    // 死区/刹车/锁定
    TIM_BreakDeadTimeConfigTypeDef bdt{};
    bdt.OffStateRunMode = cfg.offStateHighZ ? TIM_OSSR_ENABLE : TIM_OSSR_DISABLE;
    bdt.OffStateIDLEMode = cfg.offStateHighZ ? TIM_OSSI_ENABLE : TIM_OSSI_DISABLE;
    bdt.LockLevel = cfg.lockLevel;
    bdt.DeadTime = cfg.deadTime;
    bdt.BreakState = cfg.breakEnable ? TIM_BREAK_ENABLE : TIM_BREAK_DISABLE;
    bdt.BreakPolarity = cfg.breakPolarity;
    bdt.BreakFilter = 0;
    bdt.AutomaticOutput = cfg.automaticOutput ? TIM_AUTOMATICOUTPUT_ENABLE : TIM_AUTOMATICOUTPUT_DISABLE;
    HAL_TIMEx_ConfigBreakDeadTime(&handle, &bdt);
}

void TimerPwmAdv::start() {
    HAL_TIM_PWM_Start(&handle, cfg_.channel);
    if (cfg_.complement) {
        // 互补通道一起启动 两者都会置MOE 否则输出不出去
        HAL_TIMEx_PWMN_Start(&handle, cfg_.channel);
    }
}

void TimerPwmAdv::stop() {
    if (cfg_.complement) {
        HAL_TIMEx_PWMN_Stop(&handle, cfg_.channel);
    }
    HAL_TIM_PWM_Stop(&handle, cfg_.channel);
}

void TimerPwmAdv::setDeadTime(uint8_t dt) {
    MODIFY_REG(handle.Instance->BDTR, TIM_BDTR_DTG, dt);
    cfg_.deadTime = dt;
}

void TimerPwmAdv::setRepetition(uint8_t rcr) {
    handle.Instance->RCR = rcr;  // RCR是预装载 下次更新事件生效
    cfg_.repetition = rcr;
}

TimerCapture::TimerCapture(TIM_TypeDef* instance, uint32_t channel, uint16_t psc)
    : Timer(instance, psc, 0xFFFF), channel_(channel), psc_(psc) {
    configureChannelGpio(instance, channel, false);

    TIM_IC_InitTypeDef ic{};
    ic.ICPolarity = TIM_ICPOLARITY_RISING;
    ic.ICSelection = TIM_ICSELECTION_DIRECTTI;
    ic.ICPrescaler = TIM_ICPSC_DIV1;
    ic.ICFilter = 0;
    HAL_TIM_IC_ConfigChannel(&handle, &ic, channel);

    HAL_NVIC_SetPriority(irqFor(instance, true), 2, 0);
    HAL_NVIC_EnableIRQ(irqFor(instance, true));
}

void TimerCapture::start() {
    HAL_TIM_IC_Start_IT(&handle, channel_);
}

uint32_t TimerCapture::lastPeriod() const {
    return haveCapture_ ? period_ : 0;
}

float TimerCapture::frequencyHz() const {
    if (!haveCapture_ || period_ == 0) {
        return 0.0f;
    }
    return static_cast<float>(kernelClock()) / (psc_ + 1) / static_cast<float>(period_);
}

void TimerCapture::onCapture(uint32_t val) {
    if (haveCapture_) {
        // ARR=0xFFFF 计数按65536循环 处理回绕
        period_ = (val >= lastCapture_) ? (val - lastCapture_) : (val + 0x10000 - lastCapture_);
    }
    lastCapture_ = val;
    haveCapture_ = true;
}

TimerEncoder::TimerEncoder(TIM_TypeDef* instance)
    : Timer(instance, 0, 0xFFFF) {
    configureChannelGpio(instance, TIM_CHANNEL_1, false);
    configureChannelGpio(instance, TIM_CHANNEL_2, false);

    TIM_Encoder_InitTypeDef enc{};
    enc.EncoderMode = TIM_ENCODERMODE_TI12;
    enc.IC1Polarity = TIM_ICPOLARITY_RISING;
    enc.IC1Selection = TIM_ICSELECTION_DIRECTTI;
    enc.IC1Prescaler = TIM_ICPSC_DIV1;
    enc.IC1Filter = 0;
    enc.IC2Polarity = TIM_ICPOLARITY_RISING;
    enc.IC2Selection = TIM_ICSELECTION_DIRECTTI;
    enc.IC2Prescaler = TIM_ICPSC_DIV1;
    enc.IC2Filter = 0;
    HAL_TIM_Encoder_Init(&handle, &enc);
}

void TimerEncoder::start() {
    HAL_TIM_Encoder_Start(&handle, TIM_CHANNEL_ALL);
}

int16_t TimerEncoder::position() const {
    return static_cast<int16_t>(handle.Instance->CNT);
}

void TimerEncoder::setPosition(int16_t pos) {
    __HAL_TIM_SET_COUNTER(&handle, static_cast<uint16_t>(pos));
}

// 定时器中断服务程序 分发到对应Timer
void TIM1_BRK_IRQHandler(void) {
    Timer* t = Timer::fromInstance(TIM1);
    if (t != nullptr) {
        t->handleIrq();
    }
}

void TIM1_UP_IRQHandler(void) {
    Timer* t = Timer::fromInstance(TIM1);
    if (t != nullptr) {
        t->handleIrq();
    }
}

void TIM1_TRG_COM_IRQHandler(void) {
    Timer* t = Timer::fromInstance(TIM1);
    if (t != nullptr) {
        t->handleIrq();
    }
}

void TIM1_CC_IRQHandler(void) {
    Timer* t = Timer::fromInstance(TIM1);
    if (t != nullptr) {
        t->handleIrq();
    }
}

void TIM2_IRQHandler(void) {
    Timer* t = Timer::fromInstance(TIM2);
    if (t != nullptr) {
        t->handleIrq();
    }
}

void TIM3_IRQHandler(void) {
    Timer* t = Timer::fromInstance(TIM3);
    if (t != nullptr) {
        t->handleIrq();
    }
}

void TIM4_IRQHandler(void) {
    Timer* t = Timer::fromInstance(TIM4);
    if (t != nullptr) {
        t->handleIrq();
    }
}

// 更新事件回调 分发到对应Timer
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef* htim) {
    Timer* t = Timer::fromHandle(htim);
    if (t != nullptr) {
        t->onPeriodElapsed();
    }
}

// 输入捕获回调 分发到对应Timer
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef* htim) {
    Timer* t = Timer::fromHandle(htim);
    if (t != nullptr) {
        t->onCapture(__HAL_TIM_GET_COMPARE(htim, htim->Channel));
    }
}
