#include <iostream>
#include <thread>
#include <vector>
#include <iomanip>
#include <iostream>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

#include "redis_execute.hpp"
#include "redis_types.h"
#include "CLI11.hpp"

void print_map(Redis::Map& map);
void print_array(Redis::Array& array);

void print_helper(const std::string& key, Redis::redis_types* val) {
    if(val->index() == 0) { //SimpleString
        Redis::SimpleString value = std::get<Redis::SimpleString>(*val);
        LOG_INFO("{0} => {1}", key, value.get());
    }
    if(val->index() == 1) { //BulkString
        Redis::BulkString value = std::get<Redis::BulkString>(*val);
        LOG_INFO("{0} => {1}", key, value.get());
    }
    if(val->index() == 2) { //Integer
        Redis::Integer value = std::get<Redis::Integer>(*val);
        LOG_INFO("{0} => {1}", key, value.get());
    }
    if(val->index() == 3) { //Integer
        Redis::Array value = std::get<Redis::Array>(*val);
        LOG_INFO("{0} => *", key);
        print_array(value);
    }
    if(val->index() == 4) { //Integer
        Redis::Map value = std::get<Redis::Map>(*val);
        LOG_INFO("{0} => %", key);
        print_map(value);
    }
}

void print_map(Redis::Map& map) {
    for (auto& [key, val] : map.get()) {
        print_helper(key, val.get());
    }
}

void print_array(Redis::Array& array) {
    int idx_counter{0};
    for (auto& elem : array.get()) {
        print_helper(std::to_string(idx_counter), elem.get());
        idx_counter++;
    }
}

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


    output = Redis::execute(*con, "HELLO", "3");
    Redis::Map map{output};

    print_map(map);
    
    //END - Default test

    delete con;
    con = nullptr;

    return 0;
}
