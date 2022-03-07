#include "redis_client.h"

using namespace Redis;

RedisClient::RedisClient(std::string ip_address, std::string port) {
    pipe = new RedisPipe<std::string>(ip_address, port);
}

RedisClient::~RedisClient() {
    delete pipe;
    pipe = nullptr;
}

std::string RedisClient::create_command(std::string input) {
    std::string output{"*"};

    char space_delimiter = ' ';
    std::vector<std::string> words{};

    size_t pos = 0;
    while ((pos = input.find(space_delimiter)) != std::string::npos) {
        words.push_back(input.substr(0, pos));
        input.erase(0, pos + 1);
    }

    //header
    output += std::to_string(words.size()) + "\r\n";
    //content
    for (const auto &str : words) {
        output += "$" + std::to_string(str.length()) + "\r\n";
        output += str + "\r\n";
    }

    return output;
}

size_t RedisClient::get_length(std::string header) {
    size_t length{0};

    for (size_t i=1; i < header.length(); i++) {
        if (header[i] == '\r') {
            break;
        }
        length = (length * 10) + (header[i] - '0');
        
    }

    return length;
}

void RedisClient::execute(std::string input) {
    *pipe << create_command(input + ' ');
    std::string output;
    *pipe >> output;
    std::string result{""};

    switch (output[0])
    {
    case '$':
        if (output == "$-1") {
            std::cout << "Not existent" << std::endl;
            break;
        }
        *pipe >> output;
        std::cout << output << std::endl;
        break;
    case '+':
        std::cout << output << std::endl;
        break;
    case '-':
        std::cout << output << std::endl;
        break;
    default:
        break;
    }
}