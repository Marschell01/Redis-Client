#pragma once

#include <deque>
#include <sstream>

#include "redis_connection.hpp"
#include "redis_response.hpp"

namespace Redis {

    template<typename ...T>
    void execute_no_flush(RedisConnection& con, std::string operation, T ... args) {

        std::deque<std::string> cmd {
            "$" + std::to_string(operation.length()) + "\r\n" + 
            operation + "\r\n",
            (
                "$" + std::to_string(std::string{args}.length()) + "\r\n" + 
                std::string{args} + "\r\n"
            ) ...
        };

        cmd.push_front("*" + std::to_string(cmd.size()) + "\r\n");

        for (const auto& e : cmd) {
            LOG_DEBUG("Bufferend Data: {0}", e);
            con.bufferData(e);
        }
    }

    auto flush_pending(RedisConnection& con) {
        con.sendData();
        return con.getData();
    }

    template<typename ...T>
    RedisResponse execute(RedisConnection& con, std::string operation, T && ... args) {
        execute_no_flush(con, operation, args...);

        con.sendData();
        return RedisResponse{con.getData()};
    }
}