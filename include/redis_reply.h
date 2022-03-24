#pragma once

#include <string>
#include "redis_exceptions.h"
#include "logger.h"
#include <vector>
#include <map>

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

namespace Redis {

    class BaseReply {
    public:
        static ReplyType determin_type(std::string header) {
            if (header == "_") {
                return ReplyType::null;
            }
            switch (header[0]) {
            case '$':
                return ReplyType::bulk_string;
            case ':':
                return ReplyType::integer;
            case '-':
                return ReplyType::error;
            case '+':
                return ReplyType::simple_string;
            case '%':
                return ReplyType::map;
            case '*':
                return ReplyType::array;
            
            default:
                return ReplyType::no_type;
            }
        }
    };

    class BulkStringReply : public BaseReply {
    private:
        std::string content{};

    public:
        BulkStringReply(std::vector<std::string> msg) {
            msg.erase(msg.begin());
            content = msg.at(0);
        }

        std::string get_content() {
            return content;
        }

        void print_content() {
            LOG_INFO("");
        }
    };

    class IntegerReply : public BaseReply {
    private:
        int content{};

    public:
        IntegerReply(std::vector<std::string> msg) {
            content = std::stoi(msg.at(0));
        }

        int get_content() {
            return content;
        }

        void print_content() {
            LOG_INFO("");
        }
    };

    class MapReply : public BaseReply {
    private:
        std::map<std::string, std::string> content{};

    public:
        MapReply(std::vector<std::string> msg) {
            msg.erase(msg.begin());
            for (size_t size{0}; size < msg.size(); size++) {
                LOG_DEBUG("Value from messages: {0}", msg.at(size));
                if (determin_type(msg.at(size)) == ReplyType::bulk_string) {
                    size++;
                }
                std::string key{msg.at(size)};
                size++;
                if (determin_type(msg.at(size)) == ReplyType::bulk_string) {
                    size++;;
                }
                std::string value{msg.at(size)};

                content.insert(std::make_pair(key, value));
            }
        }

        void print_content() {
            for (auto const& [key, val] : content) {
                LOG_INFO("{0} => {1}", key, val);
            }
        }

        std::map<std::string, std::string> get_content() {
            return content;
        }
    };

    class ArrayReply : public BaseReply {
    private:
        std::vector<std::string> content{};

    public:
        ArrayReply(std::vector<std::string> msg) {
            msg.erase(msg.begin());
            for (size_t size{0}; size < msg.size(); size++) {
                LOG_DEBUG("Value from messages: {0}", msg.at(size));
                if (determin_type(msg.at(size)) == ReplyType::bulk_string) {
                    size++;
                }
                content.push_back(msg.at(size));
            }
        }

        void print_content() {
            for (size_t i{0}; i < content.size(); i++) {
                LOG_INFO("[{0}] : {1}", i, content.at(i));
            }
        }

        std::vector<std::string> get_content() {
            return content;
        }
    };

    template<typename T>
    class ReplyParser {

    public:
        static std::unique_ptr<T> parse(std::vector<std::string> msg) {
            return std::unique_ptr<T>(new T(msg));
        }
    };
}
