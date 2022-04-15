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

std::string interaction(const std::string& ip, const int& port, Redis::RedisClient& client) {
    std::string temp = "";
    std::vector<std::string> arguments;
    std::string input;
    std::string output;
    while (client.is_connected()) {
        temp.clear();
        arguments.clear();
        std::cout << ip << ":" << port << "> ";
        std::getline(std::cin, input);

        if (input.at(0) == '-') {
            if (input == "-end" || input == "-abort") {
                break;
            } else {
                LOG_ERROR("Invalid operation");
            }
        }

        for(size_t i={0}; i < input.length(); i++){
            if(input[i]==' '){
                arguments.push_back(temp);
                temp = "";
            } else{
                temp.push_back(input[i]);
            }  
        }
        arguments.push_back(temp);
        output = client.execute(arguments).parse<std::string>();
        std::cout << "server> " << output << std::endl;
    }
    return input;
}

int main(int argc, char* argv[]) {
    std::string ip_address{"localhost"};
    std::string lock_resource{""};
    std::string subscribe_to{""};
    std::string publish_to{""};
    int destination_port{6379};
    int wait_time{0};
    int re_run_count{1};
    bool interactive_mode_on{false};
    bool transaction_on{false};
    std::vector<std::string> commands;

    
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
    app.add_option("-c,--command", commands, "Command to execute");
    app.add_option("-r,--redo", re_run_count, "Excpects amout of itterations for command");
    app.add_option("-w,--wait", wait_time, "Specify milliseconds between command call");
    app.add_option("--subscribe", subscribe_to, "Subscribes to a specific resource");
    app.add_option("--publish", publish_to, "Publishes to a specific resource in interactive mode");
    app.add_option("-l,--lock", lock_resource ,"Sets a lock using a temporary resource and starts interactive mode");
    app.add_flag("-i,--interactive", interactive_mode_on, "Enable of disable interactive mode");
    app.add_flag("-t,--transaction", transaction_on ,"Starts a transaction and enables interactive mode");

    //LOG_SET_LOGLEVEL(LOG_LEVEL_DEBUG);

    CLI11_PARSE(app, argc, argv); 

    Redis::RedisClient client{ip_address, destination_port};
    std::string input;
    std::string output;

    if (!interactive_mode_on) {

        if (subscribe_to.size() > 0) {
            client.subscribe(subscribe_to);
            while (client.is_connected()) {
                LOG_INFO(client.is_connected());
                LOG_INFO("Subscriber waiting for data");
                    
                std::vector<Redis::RedisResponse> responses{client.fetch_data()};
                for (Redis::RedisResponse& response : responses) {
                    LOG_INFO(response.parse<std::string>());
                }
            }
            return 0;
        }

        if (publish_to.size() > 0) {
            LOG_INFO("Start publishing to: {0}", publish_to)
            while (true) {
                std::cout << "Element: ";
                std::getline(std::cin, input);
                output = client.execute("PUBLISH", publish_to, input).parse<std::string>();
                LOG_INFO(output);
            }
            return 0;
        }

        if (transaction_on) {
            client.begin_transaction();
            LOG_INFO("Type -end to end the transaction, -abort to abort the transaction");
            output = interaction(ip_address, destination_port, client);

            if (output == "-end") {
                client.end_transaction();
                return 0;
            } else if (output == "-abort") {
                client.discard_transaction();
                return 0;
            } 
        }

        if (lock_resource.size() > 0) {
            client.lock(lock_resource);
            LOG_INFO("Type -end to exit");
            interaction(ip_address, destination_port, client);
            client.unlock(lock_resource);
            return 0;
        }

        if (commands.size() > 0) {
            for (int i{0}; i < re_run_count; i++) {
                output = client.execute(commands).parse<std::string>();
                LOG_INFO(output);
                std::this_thread::sleep_for(std::chrono::milliseconds{wait_time});
            }
            return 0;
        }
    }

    //Interactive Mode
    LOG_INFO("Type -end to exit");
    interaction(ip_address, destination_port, client);

    return 0;
}
