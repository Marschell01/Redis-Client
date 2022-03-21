#include "redis_reply.h"

ReplyType Reply::determine_type(std::string msg) {
    if (msg.size() == 0) {
        throw new RedisResponseException();
    }

    ReplyType type;
    if (msg == "$-1") {
        type = ReplyType::null;
        return type;
    } 

    char determin_char = msg[0];
    switch (determin_char) {
    case '+':
        type = ReplyType::simple_string;
        break;
    case '-':
        type = ReplyType::error;
        break;
    case ':':
        type = ReplyType::integer;
        break;
    case '$':
        type = ReplyType::bulk_string;
        break;
    case '*':
        type = ReplyType::array;
        break;
    default:
        throw new RedisResponseException();
        break;
    }

    return type;
}