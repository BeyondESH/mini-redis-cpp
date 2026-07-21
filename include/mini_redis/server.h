#pragma once

#include "../include/mini_redis/resp.h"
#include "../include/mini_redis/config.hpp"
#include <vector>
#include <string>

namespace mini_redis{

struct Connection{
    Connection(std::vector<std::string> out_chunks,std::string in, int fd,size_t out_iov_idx,size_t out_offset,RespParser parser,bool is_replica);
    Connection();
    std::vector<std::string> out_chunks; //发送队列,响应客户端请求
    std::string in; //接收客户端发来的原始字节流
    int fd; //socket
    size_t out_iov_idx; //当前发送到第几块
    size_t out_offset; //块内偏移量
    RespParser parser;
    bool is_replica; //标记当前连接的客户端是不是一个“从节点

    
};

class Server{
public:
    explicit Server(const ServerConfig& config);
    ~Server();
    int run();

private:
    int setupListen();
    int setupEpoll();
    int loop();

private:
    const ServerConfig& _config;
    int _listen_fd; //监听连接描述符
    int _epoll_fd;
    int _timer_fd; //定时器文件描述符 作用：1.主动清理过期 Key 2.主从复制的心跳维持 3.AOF 缓冲区定期刷盘 
};

};