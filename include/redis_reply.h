#pragma once

#include <string>
#include "redis_exceptions.h"

enum ReplyType {
    error,
    bulk_string,
    simple_string,
    null,
    integer,
    array
};

class Reply {
public:
    static ReplyType determine_type(std::string msg);
};