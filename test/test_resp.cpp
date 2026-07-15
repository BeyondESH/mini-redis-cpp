#include <cassert>
#include <iostream>
#include "../include/mini_redis/resp.h"

using namespace mini_redis;

void testRespParser() {
    RespParser parser;

    // 1. 测试Simple String
    std::string buffer1 = "+OK\r\n";
    parser.append(buffer1); // 先把数据追加进缓冲区
    auto result1 = parser.tryParseOne();
    
    assert(result1 && result1->type == RespType::SIMPLE_STRING);
    assert(result1->bulkString == "OK");

    // 2. 测试Array
    std::string buffer2 = "*2\r\n$3\r\nSET\r\n$5\r\nmykey\r\n";
    parser.append(buffer2);
    auto result2 = parser.tryParseOne();
    
    assert(result2 && result2->type == RespType::ARRAY); // 原来是 kArray
    assert(result2->array.size() == 2);
    assert(result2->array[0].bulkString == "SET");
    assert(result2->array[1].bulkString == "mykey");
    
    std::cout << "All RESP tests passed! 🎉" << std::endl;
}