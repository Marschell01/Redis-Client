#pragma once

#include <random>

#include "redis_response.hpp"
#include "redis_connection.hpp"

namespace Redis {
    class RedisClient {
    private:
        RedisConnection* con{nullptr};
        int lock_time;
        std::string resource;
        std::string rand_key;
        bool holding_transaction{false};

        void generate_key() {
            rand_key = "";

            const std::string CHARACTERS{"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"};

            std::random_device random_device;
            std::mt19937 generator(random_device());
            std::uniform_int_distribution<> distribution(0, CHARACTERS.size() - 1);

            for (std::size_t i = 0; i < 40; ++i) {
                rand_key += CHARACTERS[distribution(generator)];
            }
        }

    public:
        RedisClient(std::string ip_address, int port) {
            try {
                con = new RedisConnection(ip_address, port);
                LOG_INFO("RedisClient::RedisClient:: Initialized");
            } catch (std::system_error& e) {
                LOG_ERROR(e.what());
                con = nullptr;
            }
        }

        ~RedisClient() {
            delete con;
            con = nullptr;
            LOG_INFO("RedisClient::RedisClient:: Shut down");
        }

        template<typename ...T>
        void execute_no_flush(std::string operation, T ... args) {
            if (con == nullptr) {
                LOG_ERROR("No connection!");
                return;
            } 

            Message request = Message::default_instance();
            std::string operation_length{"$" + std::to_string(operation.length())};
            
            std::deque<std::string> arguments {
                (
                "$" + std::to_string(std::string{args}.length()) + "\r\n" + 
                std::string{args}
                ) ...
            };

            request.add_argument("*" + std::to_string(arguments.size() + 1));
            request.add_argument(operation_length);
            request.add_argument(operation);

            for (std::string& argument : arguments) {
                request.add_argument(argument);
            }

            con->bufferProtoData(request);
        }

        RedisResponse flush_pending() {
            if (con == nullptr) {
                LOG_ERROR("No connection!");
                return RedisResponse{};
            } 

            con->sendProtoData();
            return RedisResponse{con->getProtoData().message(0)};
        }

        template<typename ...T>
        RedisResponse execute(std::string operation, T && ... args) {
            if (con == nullptr) {
                LOG_ERROR("No connection!");
                return RedisResponse{};
            } 

            execute_no_flush(operation, args...);

            con->sendProtoData();
            return RedisResponse{con->getProtoData().message(0)};
        }

        void lock() {
            if (con == nullptr) {
                LOG_ERROR("No connection!");
                return;
            } 

            generate_key();
            LOG_INFO("RedisLock::lock:: Try lock on resource: {0}", resource);
            RedisResponse output{execute("SET", resource, rand_key, "NX", "PX", "30000")};
            while (true) {
                if (output.get_type() == ReplyType::null) {
                    std::this_thread::sleep_for(std::chrono::milliseconds{1000});
                    output = execute("SET", resource, rand_key, "NX", "PX", "30000");
                } else {
                    break;
                }
            }
            LOG_INFO("RedisLock::lock:: Set lock on resource: {0}", resource);
            LOG_DEBUG("RedisLock::lock:: Key: {0}", rand_key)
        }

        void unlock() {
            if (con == nullptr) {
                LOG_ERROR("No connection!");
                return;
            } 

            RedisResponse output{execute("GET", resource)};
            LOG_INFO("RedisLock::unlock:: Try to unlock resource: {0}", resource);
            LOG_DEBUG("RedisLock::lock:: Key: {0}", rand_key)

            switch (output.get_type()) {
            case Redis::ReplyType::error: {
                break;
            }
            default:
                if (output.parse<std::string>() == rand_key) {
                    execute("DEL", resource);
                    LOG_INFO("RedisLock::unlock:: Released lock");
                } else {
                    LOG_ERROR("RedisLock::unlock:: Lock was not created by this instance");
                }
                break;
            }
        }

        bool begin_transaction(bool using_watch=false) {
            if (con == nullptr) {
                LOG_ERROR("No connection!");
                return false;
            } 

            if (using_watch) {
                execute("WATCH").parse<std::string>();
            }
            std::string resp{execute("MULTI").parse<std::string>()};
            if (resp != "+OK") {
                LOG_ERROR("Could not start transaction: {0}", resp);
                return false;
            }

            LOG_INFO("Started transaction");
            holding_transaction = true;
            return true;
        }
        
        void end_transaction() {
            if (con == nullptr) {
                LOG_ERROR("No connection!");
                return;
            } 

            if (!holding_transaction) {
                LOG_ERROR("No active transaction to end!");
                return;
            }

            std::string resp{execute("EXEC").parse<std::string>()};
            LOG_INFO(resp);
        }

        void discard_transaction() {
            if (con == nullptr) {
                LOG_ERROR("No connection!");
                return;
            } 

            if (!holding_transaction) {
                LOG_ERROR("No active transaction to discard!");
                return;
            }

            std::string resp{execute("DISCARD").parse<std::string>()};
            LOG_INFO(resp);
        }
    };
}