#pragma once

#include <map>
#include <string>

#include "redis_pipe.h"
#include "redis_reply.h"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"

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

        bool login(std::string user, std::string pwd);
    };
}