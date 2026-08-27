//
// Created by dingrui on 8/27/26.
//

#include "dbg_printf.h"

#include <stdio.h>

static UartBase* s_dbgUart = nullptr;

void dbg_printf_init(UartBase* uart) {
    s_dbgUart = uart;
    // stdout默认行缓冲 嵌入式没有tty概念 printf不会及时刷新
    // 关掉缓冲 让printf立即发送
    setvbuf(stdout, nullptr, _IONBF, 0);
}

// newlib的printf/puts最终都会调用_write syscall 重定向到串口
extern "C" int _write(int fd, const char* ptr, int len) {
    (void)fd;
    if (s_dbgUart != nullptr && len > 0) {
        s_dbgUart->send(reinterpret_cast<const uint8_t*>(ptr), static_cast<uint16_t>(len));
    }
    return len;
}
