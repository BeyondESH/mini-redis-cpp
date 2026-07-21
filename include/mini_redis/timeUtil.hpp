#pragma once 

#include <chrono>
#include <cstdint>

namespace mini_redis{

// 获取当前毫秒时间戳
inline int64_t nowMs(){
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

}