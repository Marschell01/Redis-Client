#include <iostream>
#include <thread>
#include <vector>

#include "redis_client.h"
#include "CLI11.hpp"

int main(int argc, char* argv[]) {
    std::string ip_address{"localhost"};
    std::string port{"6379"};
    std::string username{""};
    std::string password{""};
    bool with_login{false};
    
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
    app.add_flag("-l,--login", with_login, "Use if a username and password should get requested");

    CLI11_PARSE(app, argc, argv); 
    auto console = spdlog::stderr_color_mt("console");
    console->set_level(spdlog::level::trace);

    Redis::RedisClient client(ip_address, port);
    console->info("Instantiated redis client!");

    if (with_login) {
        std::cout << "username: ";
        std::getline(std::cin, username);
        std::cout << "password: ";
        std::getline(std::cin, password);
    }
    
    if (!client.login(username, password)) {
        return 1;
    }

    
    std::string input;
    std::string response;
    while (true) {
        std::cout << "Client>> ";
        std::getline(std::cin, input);
        if (input == "exit" || input == "EXIT") {
            console->info("Detected exit command!");
            console->info("Shutdown client!");
            return 0;
        }
        client.execute(input);
    }
    return 0;
}
