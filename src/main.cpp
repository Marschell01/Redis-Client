#include <iostream>
#include <thread>
#include <vector>

#include "redis_client.h"
#include "CLI11.hpp"


int main(int argc, char* argv[]) {
    std::string ip_address{"localhost"};
    std::string port{"6379"};

    CLI::App app("A simple redis client");
    app.add_option("--ip", ip_address, "ip address to connect to")->check(
      [](const std::string &str) {
            asio::error_code ec;
            asio::ip::make_address(str, ec);
            if (ec.value() != 0 && str != "localhost") {
                return std::string("Error: " + str + " invalid ip address!");
            }
            return std::string();
        }  
    );
    app.add_option("-p,--port", port, "port to connect to");

    CLI11_PARSE(app, argc, argv); 

    Redis::RedisClient client(ip_address, port);

    std::string input;
    std::string response;
    while (true) {
        std::cout << "Client>> ";
        std::getline(std::cin, input);
        client.execute(input);
    }
    return 0;
}
