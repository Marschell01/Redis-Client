#pragma once

#include <string>
#include <deque>
#include <variant>
#include <map>

#include "logger.h"

namespace Redis {

    enum ReplyType {
        simple_string,
        bulk_string,
        integer,
        error,
        null,
        array,
        map,
        no_type
    };

    class SimpleString;
    class BulkString;
    class Integer;
    class Error;
    class Array;
    class Map;

    typedef std::variant<SimpleString, BulkString, Integer, Error, Array, Map> redis_types;

    ReplyType determin_type(std::string header) {
        if (header == "$-1") {
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

    class SimpleString {
    private:
        std::string content{};

    public:
        SimpleString(std::deque<std::string>& msg) {
            LOG_DEBUG(msg.at(0));
            content = msg.at(0);
            msg.pop_front();
        }
        std::string& get() {
            return content;
        }
    };

    class BulkString  {
    private:
        std::string content{};

    public:
        BulkString(std::deque<std::string>& msg) {
            msg.pop_front();
            LOG_DEBUG(msg.at(0));
            content = msg.at(0);
            msg.pop_front();
        }

        std::string& get() {
            return content;
        }
    };

    class Integer  {
    private:
        int content{};

    public:
        Integer(std::deque<std::string>& msg) {
            LOG_DEBUG(msg.at(0));
            msg.at(0).erase(0, 1);
            content = std::stol(msg.at(0));
            msg.pop_front();
        }

        int& get() {
            return content;
        }
    };

    class Error {
    private:
        std::string content{};

    public:
        Error(std::deque<std::string>& msg) {
            LOG_DEBUG(msg.at(0));
            content = msg.at(0);
            msg.pop_front();
        }

        std::string& get() {
            return content;
        }
    };

    class Array final {
    private:
        std::deque<std::shared_ptr<redis_types>> content{};

    public:
        Array(std::deque<std::string>& msg) {
            LOG_DEBUG(msg.at(0));
            std::string header{msg.at(0)};
            header.erase(0, 1);
            LOG_DEBUG(header);
            int array_len{std::stoi(header)};

            if (array_len == 0) {
                return;
            }

            msg.pop_front();
            
            while (array_len > 0) {
                switch (determin_type(msg.at(0))) {
                    case ReplyType::simple_string:
                        content.push_front(std::make_shared<redis_types>(SimpleString(msg)));
                        break;

                    case ReplyType::bulk_string:
                        content.push_front(std::make_shared<redis_types>(BulkString(msg)));
                        break;

                    case ReplyType::integer:
                        content.push_front(std::make_shared<redis_types>(Integer(msg)));
                        break;  

                    case ReplyType::error:
                        content.push_front(std::make_shared<redis_types>(Error(msg)));
                        break;  

                    case ReplyType::array:
                        content.push_front(std::make_shared<redis_types>(Array(msg)));
                        break;   

                    default:
                        break;
                }

                array_len--;
            }
        }

        std::deque<std::shared_ptr<redis_types>>& get() {
            return content;
        }
    };

    class Map  {
    private:
        std::map<std::string, std::shared_ptr<redis_types>> content{};

    public:
        Map(std::deque<std::string>& msg) {
            LOG_DEBUG(msg.at(0));
            std::string header{msg.at(0)};
            header.erase(0, 1);
            LOG_DEBUG(header);
            int map_len{std::stoi(header)};
            msg.pop_front();
            
            std::string key{};
            while (map_len > 0) {
                key = (BulkString(msg)).get();

                switch (determin_type(msg.at(0))) {
                    case ReplyType::simple_string:
                        content.insert(std::make_pair(key, std::make_shared<redis_types>(SimpleString(msg))));
                        break;

                    case ReplyType::bulk_string:
                        content.insert(std::make_pair(key, std::make_shared<redis_types>(BulkString(msg))));
                        break;

                    case ReplyType::integer:
                        content.insert(std::make_pair(key, std::make_shared<redis_types>(Integer(msg))));
                        break;

                    case ReplyType::error:
                        content.insert(std::make_pair(key, std::make_shared<redis_types>(Error(msg))));
                        break;

                    case ReplyType::array:
                        content.insert(std::make_pair(key, std::make_shared<redis_types>(Array(msg))));
                        break;

                    case ReplyType::map:
                        content.insert(std::make_pair(key, std::make_shared<redis_types>(Map(msg))));
                        break;

                    default:
                        break;
                }
                map_len--;
            }
        }

        std::map<std::string, std::shared_ptr<redis_types>>& get() {
            return content;
        }
    };
}
