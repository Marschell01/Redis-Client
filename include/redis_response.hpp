#pragma once

#include "redis_types.hpp"

namespace Redis {

    class RedisResponse {
    private: 
        std::deque<std::string> values;
        ReplyType type;
        void map_to_string(Redis::Map map, std::string& out);
        void array_to_string(Redis::Array array, std::string& out);
        void print_helper(const std::string& key, Redis::redis_types* val, std::string& out);

    public:
        RedisResponse() {
            type = ReplyType::no_type;
        }

        RedisResponse(std::deque<std::string> values) : values{values} {
            type = determin_type(values.at(0));
        };

        ReplyType get_type();

        template<typename T>
        T parse();
    };

    ReplyType RedisResponse::get_type() {
        return type;
    }

    void RedisResponse::print_helper(const std::string& key, Redis::redis_types* val, std::string& out) {
        if(val->index() == 0) { //SimpleString
            Redis::SimpleString value = std::get<Redis::SimpleString>(*val);
            out += key + " => " + value.get() + "\n";
        }
        if(val->index() == 1) { //BulkString
            Redis::BulkString value = std::get<Redis::BulkString>(*val);
            out += key + " => " + value.get() + "\n";
        }
        if(val->index() == 2) { //Integer
            Redis::Integer value = std::get<Redis::Integer>(*val);
            out += key + " => " + std::to_string(value.get()) + "\n";
        }
        if(val->index() == 4) { //Integer
            Redis::Error value = std::get<Redis::Error>(*val);
            out += key + " => " + value.get() + "\n";
        }
        if(val->index() == 4) { //Array
            Redis::Array value = std::get<Redis::Array>(*val);
            out += key + " => *\n";
            array_to_string(value, out);
        }
        if(val->index() == 6) { //Map
            Redis::Map value = std::get<Redis::Map>(*val);
            out += key + " => %\n";
            map_to_string(value, out);
        }
    }

    void RedisResponse::map_to_string(Redis::Map map, std::string& out) {
        for (auto& [key, val] : map.get()) {
            print_helper(key, val.get(), out);
        }
    }

    void RedisResponse::array_to_string(Redis::Array array, std::string& out) {
        int idx_counter{0};
        for (auto& elem : array.get()) {
            print_helper(std::to_string(idx_counter), elem.get(), out);
            idx_counter++;
        }
    }

    // STRINGS
    template<>
    std::string RedisResponse::parse<std::string>() {
        switch (type) {
            case ReplyType::simple_string: {
                return SimpleString{values}.get();
            }
            
            case ReplyType::bulk_string: {
                return BulkString{values}.get();
            }

            case ReplyType::integer: {
                return std::to_string(Integer{values}.get());
            }

            case ReplyType::error: {
                return Error{values}.get();
            }

            case ReplyType::array: {
                std::string output{""};
                array_to_string(Redis::Array{values}, output);
                return output;
            }

            case ReplyType::map: {
                std::string output{""};
                map_to_string(Redis::Map{values}, output);
                return output;
            }

            default: {
                throw std::invalid_argument("Value can not be converted into an string!");
            }
        }
    }

    //INTEGERS
    template<>
    int RedisResponse::parse<int>() {
        if (type == ReplyType::integer) {
            return Integer{values}.get();
        }
        if (type == ReplyType::error) {
            return -1;
        }
            
        throw std::invalid_argument("Value can not be converted into an integer!");
    }

    //REDIS::ARRAYS
    template<>
    Redis::Array RedisResponse::parse<Redis::Array>() {
        if (type == ReplyType::array) {
            return Array{values};
        }

        throw std::invalid_argument("Value can not be converted into an array!");
    }

    //REDIS::MAPS
    template<>
    Redis::Map RedisResponse::parse<Redis::Map>() {
        if (type == ReplyType::map) {
            return Map{values};
        }
        
        throw std::invalid_argument("Value can not be converted into a map!");
    }
}