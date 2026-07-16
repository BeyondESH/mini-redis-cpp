#define _GNU_SOURCE

#include "../include/mini_redis/server.h"
#include <iostream>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <vector>
#include <cerrno>
#include <netinet/tcp.h>
#include <unordered_map>

namespace mini_redis{

//注册fd至epoll红黑树
int add_epoll(int epfd,int fd,uint32_t events){
    epoll_event ev{};
    ev.events=events;
    ev.data.fd=fd;
    return epoll_ctl(epfd,EPOLL_CTL_ADD,fd,&ev);
}

Connection::Connection(std::vector<std::string> out_chunks,std::string in, int fd,size_t out_iov_idx,size_t out_offset,RespParser parser,bool is_replica):out_chunks(out_chunks),in(in),fd(fd),out_iov_idx(0),out_offset(out_offset),parser(parser),is_replica(is_replica){

}
    

Connection::Connection():fd(-1),out_offset(0),out_iov_idx(0),is_replica(false){}

Server::Server(const ServerConfig &config):_config(config),_listen_fd(-1),_epoll_fd(-1),_timer_fd(-1){}

Server::~Server(){
    if(_listen_fd>=0)close(_listen_fd);
    if(_epoll_fd>=0)close(_epoll_fd);
}

int Server::setupListen(){
    //创建监听socket并设置为非阻塞
    _listen_fd= ::socket(AF_INET,SOCK_STREAM | SOCK_NONBLOCK,0);
    if(_listen_fd<0){
        std::perror("fail to create listen socket");
        return -1;
    }

    //允许服务器在重启时，立即重新绑定（bind）处于 TIME_WAIT 状态的端口，避免出现“Address already in use”（端口已被占用）的报错。
    int yes=1;
    setsockopt(_listen_fd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(yes));
    
    ::sockaddr_in address{};
    address.sin_family=AF_INET;
    address.sin_port=htons(_config.port);
    //将ip字符串转为网络字节序
    int res=::inet_pton(AF_INET,_config.address.c_str(),&address.sin_addr);
    if(res==-1){
        std::perror("not suppor AF protocol");
        return -1;
    }else if(res==0){
        std::cerr<<"the format of IP address is incorrect";
        return -1;
    }

    //绑定监听socket
    if(bind(_listen_fd,reinterpret_cast<sockaddr*>(&address),sizeof(address))<0){
        std::perror("bind failed");
        return -1;
    }
    
    //监听
    if(listen(_listen_fd,512)<0){
        std::perror("listen failed");
        return -1;
    }
    return 0;
}

int Server::setupEpoll(){
    //创建epoll fd
    _epoll_fd=epoll_create1(0);
    if(_epoll_fd<0){
        std::perror("epoll_create1 failed");
        return -1;
    }
    //注册监听fd读事件与边缘触发至epoll
    if(add_epoll(_epoll_fd,_listen_fd,EPOLL_EVENTS::EPOLLET | EPOLL_EVENTS::EPOLLIN)!=0){
        std::perror("add_epoll failed");
        return -1;
    }
    //创建定时器
    _timer_fd=timerfd_create(CLOCK_MONOTONIC,TFD_CLOEXEC | TFD_NONBLOCK);
    if(_timer_fd<0){
        std::perror("timerfd_create failed");
        return -1;
    }
    itimerspec its{};
    its.it_interval.tv_sec=0; //0秒
    its.it_interval.tv_nsec=200*1000*1000; //200ms
    its.it_value=its.it_interval; //第一次等待200ms超时，间隔200ms再次超时
    if(timerfd_settime(_timer_fd,0,&its,nullptr)<0){
        std::perror("timerfd_settime failed");
        return -1;
    }
    //注册timerfd至epoll
    if(add_epoll(_epoll_fd,_timer_fd,EPOLLIN | EPOLLET)<0){
        std::perror("add timerdf to epoll failed");
        return -1;
    }
    return 0;
}

int Server::loop(){
    std::unordered_map<int, Connection> connections;
    std::vector<epoll_event> events(128); //存放 epoll 传回的活跃事件
    while(true){
        int n=epoll_wait(_epoll_fd,events.data(),events.size(),-1);
        if(n<0){
            if(errno==EINTR) continue;
            std::perror("epoll wait failed");
            return -1;
        }
        for(int i=0;i<n;++i){
            //依次取出就绪的fd和事件
            int fd=events[i].data.fd;
            uint32_t event=events[i].events;
            //处理listen_fd连接读事件
            if(fd==_listen_fd){
                while(true){
                    sockaddr_in client_address{};
                    socklen_t len=sizeof(client_address);
                    int cfd=accept4(_listen_fd,reinterpret_cast<sockaddr *>(&client_address),&len,SOCK_NONBLOCK);
                    if(cfd<0){
                        std::perror("accept4 failed");
                        break;
                    }
                    //关闭cfd的Nagle算法
                    int open=1; //打开TCP_NODELAY
                    setsockopt(cfd,IPPROTO_TCP,TCP_NODELAY,&open,sizeof(open));
                    add_epoll(_epoll_fd,cfd,EPOLL_EVENTS::EPOLLIN | EPOLLET | EPOLLRDHUP | EPOLLHUP);
                    connections.emplace(cfd,Connection{std::vector<std::string>{},std::string(),cfd,0,0,RespParser{},false});
                }
                continue;
            }
            //处理timerfd
            
        }
    }
 }

};

