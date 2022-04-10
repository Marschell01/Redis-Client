#pragma once

#include <random>

#include "redis_response.hpp"
#include "redis_connection.hpp"

namespace Redis {
    class RedisClient {
    private:
        RedisConnection* con{nullptr};
        int lock_time;
        std::string rand_key;
        bool holding_transaction{false};

        void generate_key() {
            rand_key = "";

            const std::string CHARACTERS{"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"};

            std::random_device random_device;
            std::mt19937 generator(random_device());
            std::uniform_int_distribution<> distribution(0, CHARACTERS.size() - 1);

            for (std::size_t i{0}; i < 40; ++i) {
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

        bool is_connected() {
            return con != nullptr;
        }

        template<typename ...T>
        bool execute_no_flush(std::string operation, T ... args) {
            if (con == nullptr) {
                LOG_ERROR("execute_no_flush:: No connection!");
                return false;
            } 

            try {
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

                con->buffer_proto_data(request);
                return true;
            } catch(std::system_error& e) {
                LOG_ERROR("execute_no_flush:: Connection got aborted!");
                return false;
            }
        }

        std::vector<RedisResponse> flush_pending() {
            if (con == nullptr) {
                LOG_ERROR("flush_pending:: No connection!");
                return std::vector<RedisResponse>{};
            } 
            try {
                con->send_proto_data();
                Message msg{con->get_proto_data().message(0)};

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
            } catch(std::system_error& e) {
                LOG_ERROR("flush_pending:: Connection got aborted!");
            }
            
        }

        template<typename ...T>
        RedisResponse execute(std::string operation, T && ... args) {
            if (con == nullptr) {
                LOG_ERROR("execute:: No connection!");
                return RedisResponse{};
            } 
            if (!execute_no_flush(operation, args...)) {
                return RedisResponse{};
            }
            
            try {
                con->send_proto_data();
                Message msg{con->get_proto_data().message(0)};
                LOG_DEBUG("execute:: bevore deque");
                std::deque<std::string> values;
                for (int i{0}; i < msg.argument_size(); i++) {
                    LOG_DEBUG("execute:: value: deque-value: {0}", msg.argument(i));
                    values.push_back(msg.argument(i));
                }
                LOG_DEBUG("execute:: after deque");
                return RedisResponse{values};
            } catch(std::system_error& e) {
                LOG_ERROR("execute:: Connection got aborted!");
            }
            return RedisResponse{};
        }

        void subscribe(std::string sub_object) {
            execute_no_flush("SUBSCRIBE", sub_object);
            con->send_proto_with_mode("SUB");
        }

        std::vector<RedisResponse> fetch_data() {
            try {
                con->send_proto_with_mode("ACK");
                Message msg{con->get_proto_data().message(0)};
                std::deque<std::string> values;
                std::vector<RedisResponse> responses;
                values.clear();

                for (int i{0}; i < msg.argument_size(); i++) {
                    LOG_DEBUG("fetch_data:: value: deque-value: {0}", msg.argument(i));
                    values.push_back(msg.argument(i));
                }

                while (values.size() > 0) {
                    RedisResponse resp{values};
                    responses.push_back(resp);
                    values.erase(values.begin(), values.begin() + resp.get_size());
                }
                
                return responses;
            } catch(std::system_error& e) {
                LOG_ERROR("execute:: Connection got aborted!");
            }
            return std::vector<RedisResponse>{};
        }

        bool lock(std::string resource) {
            if (con == nullptr) {
                LOG_ERROR("execute:: No connection!");
                return false;
            }

            generate_key();
            LOG_INFO("lock:: Try lock on resource: {0}", resource);
            RedisResponse output{execute("SET", resource, rand_key, "NX", "PX", "30000")};
            while (true) {
                if (output.get_type() == ReplyType::null) {
                    std::this_thread::sleep_for(std::chrono::milliseconds{1000});
                    output = execute("SET", resource, rand_key, "NX", "PX", "30000");
                }
                else if (output.get_type() == ReplyType::no_type) {
                    LOG_ERROR("lock:: response size was 0!");
                    return false;
                } else {
                    break;
                }
            }
            LOG_INFO("lock:: Set lock on resource: {0}", resource);
            LOG_DEBUG("lock:: Key: {0}", rand_key);
            return true;
        }

        bool unlock(std::string resource) {
            if (con == nullptr) {
                LOG_ERROR("unlock:: No connection!");
                return false;
            } 

            RedisResponse output{execute("GET", resource)};
            if (output.get_type() == ReplyType::no_type) {
                LOG_ERROR("lock:: response size was 0!");
                return false;
            }
            LOG_INFO("unlock:: Try to unlock resource: {0}", resource);
            LOG_DEBUG("unlock:: Random key is: {0}", rand_key)

            switch (output.get_type()) {
            case Redis::ReplyType::error: {
                break;
            }
            default:
                if (output.parse<std::string>() == rand_key) {
                    RedisResponse resp{execute("DEL", resource)};
                    if (resp.get_type() == ReplyType::no_type) {
                        LOG_ERROR("lock:: response size was 0!");
                        return false;
                    }
                    LOG_INFO("unlock:: Released lock");
                    return true;
                    
                } else {
                    LOG_ERROR("unlock:: Lock was not created by this instance");
                }
                break;
            }
            return false;
        }

        bool begin_transaction(bool using_watch=false) {
            if (con == nullptr) {
                LOG_ERROR("begin_transaction:: No connection!");
                return false;
            } 

            if (using_watch) {
                RedisResponse resp{execute("WATCH")};
                if (resp.get_type() == ReplyType::no_type) {
                    LOG_ERROR("lock:: response size was 0!");
                    return false;
                }
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
        
        bool end_transaction() {
            if (con == nullptr) {
                LOG_ERROR("end_transaction:: No connection!");
                return false;
            } 

            if (!holding_transaction) {
                LOG_ERROR("end_transaction:: No active transaction to end!");
                return false;
            }

            RedisResponse resp{execute("EXEC")};
            if (resp.get_type() == ReplyType::no_type) {
                LOG_ERROR("lock:: response size was 0!");
                return false;
            }
            LOG_INFO(resp.parse<std::string>());
            return true;
        }

        bool discard_transaction() {
            if (con == nullptr) {
                LOG_ERROR("discard_transaction:: No connection!");
                return false;
            } 

            if (!holding_transaction) {
                LOG_ERROR("discard_transaction:: No active transaction to discard!");
                return false;
            }

            RedisResponse resp{execute("DISCARD")};
            if (resp.get_type() == ReplyType::no_type) {
                LOG_ERROR("lock:: response size was 0!");
                return false;
            }
            LOG_INFO(resp.parse<std::string>());
            return true;
        }
    };
}