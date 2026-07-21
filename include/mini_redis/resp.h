#pragma once

#include <string>
#include <vector>
#include <cstddef>
#include <optional>

namespace mini_redis{

/*
 * RESP协议类型
 * +xxx\r\n   Simple String
 * -xxx\r\n   Error
 * :xxx\r\n   Integer
 * $xxx\r\n   Bulk String
 * *xxx\r\n   Array
 * $-1\r\n，  Null
 */

enum class RespType{
    SIMPLE_STRING,
    ERROR,
    INTEGER,
    BULK_STRING,
    ARRAY,
    NIL
};

struct RespValue {
    RespType type=RespType::NIL;
    std::string bulkString;
    std::vector<RespValue> array;
};

class RespParser{
public:
    // 将新读取的网络字节流追加到内部缓冲区中
    void append(std::string_view data);
    // 尝试解析出一个完整的 RESP 值。
    // 如果缓冲区数据不足以构成一个完整的数据包，返回 std::nullopt；
    // 如果解析过程中遇到协议错误，返回一个 type 为 kError 且 bulk 存有错误信息的 RespValue 对象。
    std::optional<RespValue> tryParseOne();
    // 尝试解析出一个完整的 RESP 值，并额外返回该值在协议中对应的原始字节（用于透传、日志或调试）
    std::optional<std::pair<RespValue,std::string>> tryParseOneWithRaw();

private:
    std::string _buffer;
    // 解析单行数据（寻找 \r\n 并截取，pos 会被更新为下一行的起点，out_line 输出截取到的内容）
    bool parseLine(size_t& pos, std::string& out_line);
    // 解析整数（常用于解析数组长度或 Bulk String 的长度）
    bool parseInteger(size_t& pos, int64_t& out_value);
    // 解析 Bulk String ($ 开头的数据)
    bool parseBulkString(size_t& pos, RespValue& out);
    // 解析 Simple String (+ 开头), Error (- 开头) 或 Integer (: 开头) 
    bool parseSimple(size_t& pos, RespType t, RespValue& out);
    // 解析数组 (* 开头，内部会递归调用其他 parse 函数)
    bool parseArray(size_t& pos, RespValue& out);
};

//将string序列化为resp格式string
std::string respSimpleString(std::string_view str);
std::string respError(std::string_view str);
std::string respBulk(std::string_view str);
std::string respNullBulk();
std::string respInteger(int64_t str);

}