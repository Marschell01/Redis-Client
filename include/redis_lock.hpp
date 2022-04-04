#pragma once

#include <string>
#include <fstream>
#include <random>

#include "logger.h"
#include "redis_execute.hpp"
#include "redis_connection.hpp"

namespace Redis {

    class RedisLock {
    private:
        RedisConnection& con;
        int lock_time;
        std::string resource;
        std::string rand_key;

        void generate_key() {
            rand_key = "";

            const std::string CHARACTERS{"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"};

            std::random_device random_device;
            std::mt19937 generator(random_device());
            std::uniform_int_distribution<> distribution(0, CHARACTERS.size() - 1);

            for (std::size_t i = 0; i < 40; ++i)
            {
                rand_key += CHARACTERS[distribution(generator)];
            }
        }

    public:
        RedisLock(RedisConnection& con, std::string resource) : con{con}, resource{resource} {
            generate_key();
        }

        void lock() {
            LOG_INFO("RedisLock::lock:: Try lock on resource: {0}", resource);
            RedisResponse output{execute(con, "SET", resource, rand_key, "NX", "PX", "30000")};
            while (true) {
                if (output.get_type() == ReplyType::null) {
                    std::this_thread::sleep_for(std::chrono::milliseconds{1000});
                    output = execute(con, "SET", resource, rand_key, "NX", "PX", "30000");
                } else {
                    break;
                }
            }
            LOG_INFO("RedisLock::lock:: Set lock on resource: {0}", resource);
            LOG_DEBUG("RedisLock::lock:: Key: {0}", rand_key)
        }

        void unlock() {
            RedisResponse output{execute(con, "GET", resource)};
            LOG_INFO("RedisLock::unlock:: Try to unlock resource: {0}", resource);
            LOG_DEBUG("RedisLock::lock:: Key: {0}", rand_key)

            switch (output.get_type()) {
            case Redis::ReplyType::error: {
                break;
            }
            default:
                if (output.parse<std::string>() == rand_key) {
                    execute(con, "DEL", resource);
                    LOG_INFO("RedisLock::unlock:: Released lock");
                } else {
                    LOG_ERROR("RedisLock::unlock:: Lock was not created by this instance");
                }
                break;
            }
        }
    };
}