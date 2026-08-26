//
// Created by dingrui on 8/27/26.
//

#pragma once

#include "uart.h"

// 注册调试串口 之后printf/puts都会从该串口发出
// 需在创建好UART对象后调用 如 dbg_printf_init(&uart1_poll);
void dbg_printf_init(UartBase* uart);
