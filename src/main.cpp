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
    );
    app.add_option("-p,--port", destination_port, "port to connect to");

    //LOG_SET_LOGLEVEL(LOG_LEVEL_DEBUG);

    CLI11_PARSE(app, argc, argv); 



    //Interactive Mode
    std::string input;
    std::string output;
    Redis::RedisClient client{ip_address, destination_port};
    while (true) {
        std::cout << ip_address << ":" << destination_port << "> ";
        std::getline(std::cin, input);
        std::string temp = "";
        std::vector<std::string> arguments;
        for(size_t i={0}; i < input.length(); i++){
            
            if(input[i]==' '){
                arguments.push_back(temp);
                temp = "";
            }
            else{
                temp.push_back(input[i]);
            }
            
        }
        arguments.push_back(temp);

        output = client.execute(arguments).parse<std::string>();
        std::cout << "server> " << output << std::endl;
    }

    return 0;
}
