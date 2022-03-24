#pragma once

#include <map>
#include <string>

#include "redis_connection.hpp"
#include "redis_reply.h"
#include "logger.h"

namespace Redis {

    class RedisClient {
    private:
        RedisConnection* con{nullptr};
    public:
        RedisClient(std::string ip_address, std::string port);
        ~RedisClient();

        size_t get_length(std::string header);
        std::string create_command(std::string input);
        void execute(std::string input);

        bool login(std::string user, std::string pwd);
    };
}