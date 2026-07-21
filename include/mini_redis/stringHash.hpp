#pragma once

#include <string_view>
#include <functional>

namespace mini_redis{
    // 透明哈希：让 unordered_map<std::string, ...> 可以用 string_view 直接查找
    struct StringHash{
        // 声明 is_transparent 表示这是一个透明哈希函数
        using is_transparent=void;
        // 关键：这个 dummy 成员防止 libstdc++ 把空 hasher 判为 "fast hash"
        // 如果没有它，libstdc++ 会禁用异构查找优化
        char _dummy=0;
        
        template<typename T>
        size_t operator()(const T& val) const noexcept{
            return std::hash<std::string_view>{}(std::string_view(val));
        }
    };

    // 透明相等比较
    struct StringEqual{
        using is_transparen=void;

        template<typename T,typename U>
        bool operator()(const T& a, const U& b)const noexcept{
            return std::string_view(a)==std::string_view(b);
        }
    };
}