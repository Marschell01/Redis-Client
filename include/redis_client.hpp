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
                LOG_INFO("RedisClient:: Initialized");
            } catch (std::system_error& e) {
                LOG_ERROR(e.what());
                con = nullptr;
            }
        }

        ~RedisClient() {
            delete con;
            con = nullptr;
            LOG_INFO("RedisClient:: Shut down");
        }

        template<typename ...T>
        void execute_no_flush(std::string operation, T ... args) {
            if (con == nullptr) {
                LOG_ERROR("execute_no_flush:: No connection!");
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

        std::vector<RedisResponse> flush_pending() {
            if (con == nullptr) {
                LOG_ERROR("flush_pending:: No connection!");
                return std::vector<RedisResponse>{};
            } 

            con->sendProtoData();
            Message msg{con->getProtoData().message(0)};

            std::deque<std::string> values;
            std::vector<RedisResponse> responses;
            for (int i{0}; i < msg.argument_size(); i++) {
                LOG_DEBUG("flush_pending:: {0}", msg.argument(i));
                values.push_back(msg.argument(i));
            }

            while (values.size() > 0) {
                RedisResponse resp{values};
                responses.push_back(resp);
                values.erase(values.begin(), values.begin() + resp.get_size());
            }
            return responses;
        }

        template<typename ...T>
        RedisResponse execute(std::string operation, T && ... args) {
            if (con == nullptr) {
                LOG_ERROR("execute:: No connection!");
                return RedisResponse{};
            } 

            execute_no_flush(operation, args...);

            con->sendProtoData();
            Message msg{con->getProtoData().message(0)};
            LOG_DEBUG("execute:: bevore deque");
            std::deque<std::string> values;
            for (int i{0}; i < msg.argument_size(); i++) {
                values.push_back(msg.argument(i));
            }
            LOG_DEBUG("execute:: after deque");
            return RedisResponse{values};
        }

        void lock() {
            if (con == nullptr) {
                LOG_ERROR("execute:: No connection!");
                return;
            } 

            generate_key();
            LOG_INFO("lock:: Try lock on resource: {0}", resource);
            RedisResponse output{execute("SET", resource, rand_key, "NX", "PX", "30000")};
            while (true) {
                if (output.get_type() == ReplyType::null) {
                    std::this_thread::sleep_for(std::chrono::milliseconds{1000});
                    output = execute("SET", resource, rand_key, "NX", "PX", "30000");
                } else {
                    break;
                }
            }
            LOG_INFO("lock:: Set lock on resource: {0}", resource);
            LOG_DEBUG("lock:: Key: {0}", rand_key)
        }

        void unlock() {
            if (con == nullptr) {
                LOG_ERROR("unlock:: No connection!");
                return;
            } 

            RedisResponse output{execute("GET", resource)};
            LOG_INFO("unlock:: Try to unlock resource: {0}", resource);
            LOG_DEBUG("unlock:: Random key is: {0}", rand_key)

            switch (output.get_type()) {
            case Redis::ReplyType::error: {
                break;
            }
            default:
                if (output.parse<std::string>() == rand_key) {
                    execute("DEL", resource);
                    LOG_INFO("unlock:: Released lock");
                } else {
                    LOG_ERROR("unlock:: Lock was not created by this instance");
                }
                break;
            }
        }

        bool begin_transaction(bool using_watch=false) {
            if (con == nullptr) {
                LOG_ERROR("begin_transaction:: No connection!");
                return false;
            } 

            if (using_watch) {
                execute("WATCH").parse<std::string>();
            }
            std::string resp{execute("MULTI").parse<std::string>()};
            if (resp != "+OK") {
                LOG_ERROR("begin_transaction:: Could not start transaction: {0}", resp);
                return false;
            }

            LOG_INFO("begin_transaction:: Started transaction");
            holding_transaction = true;
            return true;
        }
        
        void end_transaction() {
            if (con == nullptr) {
                LOG_ERROR("end_transaction:: No connection!");
                return;
            } 

            if (!holding_transaction) {
                LOG_ERROR("end_transaction:: No active transaction to end!");
                return;
            }

            std::string resp{execute("EXEC").parse<std::string>()};
            LOG_INFO(resp);
        }

        void discard_transaction() {
            if (con == nullptr) {
                LOG_ERROR("discard_transaction:: No connection!");
                return;
            } 

            if (!holding_transaction) {
                LOG_ERROR("discard_transaction:: No active transaction to discard!");
                return;
            }

            std::string resp{execute("DISCARD").parse<std::string>()};
            LOG_INFO(resp);
        }
    };
}