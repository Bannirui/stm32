//
// Created by dingrui on 8/23/26.
//

#pragma once

#include "stm32f103xb.h"

class Led {
public:
    Led(GPIO_TypeDef *port, uint16_t pin);

    ~Led() = default;

    void toggleMs(unsigned long ms);

private:
    GPIO_TypeDef *port;
    uint16_t pin;
};
