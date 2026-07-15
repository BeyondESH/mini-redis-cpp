#pragma once

#include <cstdint>
#include <string>

namespace mini_redis{

struct ServerConfig{
    uint16_t port=6388;
    std::string address="0.0.0.0";
    
};

};