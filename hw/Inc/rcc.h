//
// Created by dingrui on 8/23/26.
//

#pragma once

#include <cstdint>

// 时钟
class Rcc {
public:
    virtual ~Rcc() = default;

protected:
    // 时钟源
    enum class Source : uint8_t {
        Hsi, // 内部高速时钟
        Hse, // 外部晶振
    };

    // 配振荡器->配总线分频
    explicit Rcc(Source source);

    Rcc() = default;
};

/**
 * 内部HSI通过PLL倍频到64M
 */
class RccLock64 : public Rcc {
public:
    RccLock64();
};

/**
 * 外部晶振HSE通过PLL倍频到72M
 */
class RccLock72 : public Rcc {
public:
    RccLock72();
};
