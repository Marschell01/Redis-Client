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

    LOG_SET_LOGLEVEL(LOG_LEVEL_DEBUG);

    CLI11_PARSE(app, argc, argv); 

    LOG_INFO("Starting redis client");
    try {
        Redis::RedisClient client{"localhost", "6379"};
        LOG_INFO("Started redis client");

        LOG_INFO("Starting login procedure");
        if (with_login) {
            std::cout << "username: ";
            std::getline(std::cin, username);
            std::cout << "password: ";
            std::getline(std::cin, password);
        }
        
        if (!client.login(username, password)) {
            LOG_ERROR("Invalid credentials for login");
            return 1;
        }

        LOG_INFO("Finished login procedure");

        
        std::string input{};
        while (true) {
            std::cout << "Client>> ";
            std::getline(std::cin, input);
            if (input == "exit" || input == "EXIT") {
                LOG_INFO("Detected exit command!");
                LOG_INFO("Shutdown client!");
                return 0;
            }
            client.execute(input);
        }
        
    } catch (asio::system_error& e) {
        LOG_ERROR(e.what());
        return 1;
    }
    return 0;
}
