#include <iostream>
#include <thread>
#include <vector>

#include "redis_execute.hpp"
#include "redis_types.h"
#include "CLI11.hpp"

int main(int argc, char* argv[]) {
    std::string ip_address{"localhost"};
    std::string port{"6379"};
    std::string username{""};
    std::string password{""};
    bool with_login{false};

    std::deque<std::string> output{};
    
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

    Redis::RedisConnection* con{new Redis::RedisConnection(ip_address, port)};

    //START - Pipeline test
    Redis::execute_no_flush(*con, "PING");
    Redis::execute_no_flush(*con, "PING");
    Redis::execute_no_flush(*con, "PING");

    output = Redis::flush_pending(*con);


    for (int i{0}; i < 3; i++) {
        Redis::SimpleString str{std::ref(output)};
        LOG_INFO("Output: {0}", str.get());
        LOG_DEBUG("Queue length: {0}", output.size());
    }
    //END - Pipeline test
 
    //START - Default test
    output = Redis::execute(*con, "SET", "name", "Max Mustermann");
    LOG_INFO(Redis::SimpleString(output).get());
    output = Redis::execute(*con, "GET", "name");
    LOG_INFO(Redis::BulkString(output).get());
    //END - Default test

    delete con;
    con = nullptr;

    return 0;
}
