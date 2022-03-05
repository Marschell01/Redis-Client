#pragma once

#include "redis_pipe.h"

namespace Redis {

    class RedisClient {
    private:
        RedisPipe<std::string>* pipe; 
    public:
        RedisClient(std::string ip_address, std::string port);
        ~RedisClient();
        
        bool set(std::string key, std::string value);
        std::string get(std::string key);
    };
}