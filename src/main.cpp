
#include <iostream>
#include <thread>

#include "redis_pipe.h"

int main() {
    RedisClient::RedisPipe<std::string> pipe("localhost", "6379");


    pipe << "HELLO\r\n";
    std::string output;
    pipe >> output;
    std::string output_length{output.substr(output.find("*")+1)};
    int counter{0};

    while (counter < std::stoi(output_length)) {
        pipe >> output;
        if (output[0] != '$') {
            std::cout << output << std::endl;
            counter++;
        }
    }


    return 0;
}
