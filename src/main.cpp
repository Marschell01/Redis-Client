/*
author: Dinhof Marcel
matnr: i17044
file: main.cpp
desc: This file implements the redis clients launch and interface
date: 2022-04-09
class: 5b
catnr: 3
*/

#include <string>
#include <chrono>

#include "CLI11.hpp"
#include "redis_client.hpp"
#include "proxy.hpp"

int main(int argc, char* argv[]) {
    std::string ip_address{"localhost"};
    int destination_port{6379};
    
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
    )->required();
    app.add_option("-p,--port", destination_port, "port to connect to")->required();

    //LOG_SET_LOGLEVEL(LOG_LEVEL_DEBUG);

    CLI11_PARSE(app, argc, argv); 
    return 0;
}
