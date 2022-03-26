#pragma once

#include <string>
#include "redis_exceptions.h"
#include "logger.h"
#include <deque>
#include <variant>

namespace Redis {

    enum ReplyType {
        error,
        bulk_string,
        simple_string,
        null,
        integer,
        array,
        map,
        no_type
    };
    

    ReplyType determin_type(std::string header) {
        if (header == "_") {
            return ReplyType::null;
        }
        switch (header[0]) {
        case '+':
            return ReplyType::simple_string;
        case '$':
            return ReplyType::bulk_string;
        case ':':
            return ReplyType::integer;
        case '%':
            return ReplyType::map;
        case '*':
            return ReplyType::array;
        case '-':
            return ReplyType::error;
        
        default:
            return ReplyType::no_type;
        }
    }


    class SimpleString  {
    private:
        std::string content{};

    public:
        SimpleString(std::deque<std::string>& msg) {
            content = msg.at(0);
            msg.pop_front();
        }

        const std::string& get() {
            return content;
        }
    };

    class BulkString  {
    private:
        std::string content{};

    public:
        BulkString(std::deque<std::string>& msg) {
            msg.pop_front();
            content = msg.at(0);
            msg.pop_front();
        }

        const std::string& get() {
            return content;
        }
    };

    class Integer  {
    private:
        int content{};

    public:
        Integer(std::deque<std::string>& msg) {
            content = std::stoi(msg.at(0));
            msg.pop_front();
        }

        const int& get() {
            return content;
        }
    };

    class Map  {
    private:
        std::deque<std::variant<SimpleString, BulkString, Integer>> content{};

    public:
        Map(std::deque<std::string>& msg) {
            int map_len{std::stoi(msg.at(0))};
            msg.pop_front();
            
            while (map_len > 0) {
                switch (determin_type(msg.at(0)))
                {
                case ReplyType::simple_string:
                    content.push_front(SimpleString(msg));
                    break;
                case ReplyType::bulk_string:
                    content.push_front(BulkString(msg));
                    break;
                case ReplyType::integer:
                    content.push_front(Integer(msg));
                    break;
                
                default:
                    break;
                }

                map_len--;
            }
        }

        const std::deque<std::variant<SimpleString, BulkString, Integer>>& get() {
            return content;
        }
    };
}
