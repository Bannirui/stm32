//
// Created by dingrui on 8/25/26.
//

#pragma once

#include <stdint.h>

// 环形缓冲 中断写(生产者) 主循环读(消费者)
class RingBuffer {
public:
    // 写入一个字节 满了丢弃最旧的数据
    void write(uint8_t byte) {
        data_[w_] = byte;
        w_ = (w_ + 1) % kSize;
        if (w_ == r_) {
            // 满了 丢掉缓冲区的老数据
            r_ = (r_ + 1) % kSize;
        }
    }

    // 读出一个字节 空返回false
    bool read(uint8_t &byte) {
        // 缓冲区是空的情况
        if (r_ == w_) { return false; }
        byte = data_[r_];
        r_ = (r_ + 1) % kSize;
        return true;
    }

    // 可读字节数
    uint16_t available() const {
        return (w_ + kSize - r_) % kSize;
    }

    // 是否为空
    bool empty() const {
        return r_ == w_;
    }

    // 清空
    void clear() {
        r_ = 0;
        w_ = 0;
    }

private:
    static constexpr uint16_t kSize = 256;
    uint8_t data_[kSize];
    // [...w)是已经读取过的 [w...r)是可读的 [r...]是可写的
    // 写指针
    uint16_t r_ = 0;
    // 读指针
    uint16_t w_ = 0;
};
