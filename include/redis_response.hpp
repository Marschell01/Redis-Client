/*
author: Dinhof Marcel
matnr: i17044
file: redis_response.hpp
desc: This module implements the response parsing for the client
date: 2022-04-07
class: 5b
catnr: 3
*/

#pragma once

#include "redis_types.hpp"
#include "redis.pb.h"

namespace Redis {

    class RedisResponse {
    private: 
        std::deque<std::string> values;
        ReplyType type;
        int size{0};
        void map_to_string(Redis::Map map, std::string& out);
        void array_to_string(Redis::Array array, std::string& out);
        void print_helper(const std::string& key, Redis::redis_types* val, std::string& out);

    public:
        RedisResponse() {
            type = ReplyType::no_type;
        }

        RedisResponse(std::deque<std::string> values) : values{values} {
            type = determin_type(values.at(0), size);
            LOG_DEBUG("RedisResponse:: determinded type");
        }

        const int& get_size() {
            return size;
        }

        ReplyType get_type();

        template<typename T>
        T parse();
    };

    ReplyType RedisResponse::get_type() {
        return type;
    }

    //Helper function to print an array or map
    void RedisResponse::print_helper(const std::string& key, Redis::redis_types* val, std::string& out) {
        LOG_DEBUG("print_helper:: Variant index: {0}", val->index());
        if(val->index() == 0) { //SimpleString
            LOG_DEBUG("print_helper:: went into simple string");
            Redis::SimpleString value = std::get<Redis::SimpleString>(*val);
            out += key + " => " + value.get() + "\n";
        }
        if(val->index() == 1) { //BulkString
            LOG_DEBUG("print_helper:: went into bulk string");
            Redis::BulkString value = std::get<Redis::BulkString>(*val);
            out += key + " => " + value.get() + "\n";
        }
        if(val->index() == 2) { //Integer
            LOG_DEBUG("print_helper:: went into integer");
            Redis::Integer value = std::get<Redis::Integer>(*val);
            out += key + " => " + std::to_string(value.get()) + "\n";
        }
        if(val->index() == 3) { //Error
            LOG_DEBUG("print_helper:: went into error");
            Redis::Error value = std::get<Redis::Error>(*val);
            out += key + " => " + value.get() + "\n";
        }
        if(val->index() == 4) { //Array
            LOG_DEBUG("print_helper:: went into array");
            Redis::Array value = std::get<Redis::Array>(*val);
            out += key + " => *\n";
            array_to_string(value, out);
        }
        if(val->index() == 6) { //Map
            LOG_DEBUG("print_helper:: went into map");
            Redis::Map value = std::get<Redis::Map>(*val);
            out += key + " => %\n";
            map_to_string(value, out);
        }
    }

    //printing of a map
    void RedisResponse::map_to_string(Redis::Map map, std::string& out) {
        LOG_DEBUG("map_to_string:: bevore loop");
        for (auto& [key, val] : map.get()) {
            print_helper(key, val.get(), out);
        }
    }

    //printing of an array
    void RedisResponse::array_to_string(Redis::Array array, std::string& out) {
        int idx_counter{0};
        for (auto& elem : array.get()) {
            print_helper(std::to_string(idx_counter), elem.get(), out);
            idx_counter++;
        }
    }

    // parsing of strings
    template<>
    std::string RedisResponse::parse<std::string>() {
        switch (type) {
            case ReplyType::simple_string: {
                LOG_DEBUG("parse<std::string>:: simple string");
                return SimpleString{values}.get();
            }
            
            case ReplyType::bulk_string: {
                LOG_DEBUG("parse<std::string>:: bulk string");
                return BulkString{values}.get();
            }

            case ReplyType::integer: {
                LOG_DEBUG("parse<std::string>:: integer");
                return std::to_string(Integer{values}.get());
            }

            case ReplyType::error: {
                LOG_DEBUG("parse<std::string>:: error");
                return Error{values}.get();
            }

            case ReplyType::array: {
                LOG_DEBUG("parse<std::string>:: array");
                std::string output{""};
                array_to_string(Redis::Array{values}, output);
                return output;
            }

            case ReplyType::map: {
                LOG_DEBUG("parse<std::string>:: map");
                std::string output{""};
                map_to_string(Redis::Map{values}, output);
                return output;
            }

            case ReplyType::null: {
                LOG_DEBUG("parse<std::string> null");
                return "";
            }

            default: {
                throw std::invalid_argument("Value can not be converted into an string!");
            }
        }
    }

    //parsing of integers
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

    //parsing of arrays
    template<>
    Redis::Array RedisResponse::parse<Redis::Array>() {
        if (type == ReplyType::array) {
            return Array{values};
        }

        throw std::invalid_argument("Value can not be converted into an array!");
    }

    //parsing of maps
    template<>
    Redis::Map RedisResponse::parse<Redis::Map>() {
        if (type == ReplyType::map) {
            return Map{values};
        }
        
        throw std::invalid_argument("Value can not be converted into a map!");
    }
}