#include "redis_client.h"

using namespace Redis;

RedisClient::RedisClient(std::string ip_address, std::string port) {
    pipe = new RedisPipe<std::string>(ip_address, port);
}

RedisClient::~RedisClient() {
    delete pipe;
    pipe = nullptr;
}

bool RedisClient::set(std::string key, std::string value) {
    std::string input{"SET " + key + " " + value + "\r\n"};
    *pipe << input;

    std::string output;
    *pipe >> output;

    if (output[0] == '+') {
        std::cerr << "Entry added" << std::endl;
        return true;
    } else {
        std::cerr << "Entry could not be added!" << std::endl;
        return false;
    }
}

std::string RedisClient::get(std::string key) {
    std::string input{"GET " + key + "\r\n"};
    *pipe << input;
    std::string output;
    *pipe >> output;
    
    if (output == "$-1") {
        std::cerr << "Entry not found" << std::endl;
        return "";
    }
    *pipe >> output;
    return output;
}