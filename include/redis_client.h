#pragma once

#include <map>

#include "redis_pipe.h"

namespace Redis {

    class RedisClient {
    private:
        RedisPipe<std::string>* pipe; 
    public:
        RedisClient(std::string ip_address, std::string port);
        ~RedisClient();

        size_t get_length(std::string header);
        std::string create_command(std::string input);
        void execute(std::string input);
    };
}