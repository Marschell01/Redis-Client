#include <string>

#include "redis_execute.hpp"
#include "redis_types.h"
#include "CLI11.hpp"
#include "redis_lock.hpp"
#include "redis_transaction.hpp"

#include <chrono>

int main(int argc, char* argv[]) {
    std::string ip_address{"localhost"};
    std::string port{"6379"};
    std::string subscribe_to{""};
    std::string publish_to{""};
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
    app.add_option("--subscriber", subscribe_to, "Specify element for subscription");
    app.add_option("--publisher", publish_to, "Specify element to publish to");

    //LOG_SET_LOGLEVEL(LOG_LEVEL_DEBUG);

    CLI11_PARSE(app, argc, argv); 

    /*
    //START - Pipeline test
    Redis::RedisConnection con{ip_address, port};

    Redis::execute_no_flush(con, "PING");
    LOG_INFO("Sent PING");
    Redis::execute_no_flush(con, "PING");
    LOG_INFO("Sent PING");
    Redis::execute_no_flush(con, "PING");
    LOG_INFO("Sent PING");

    Redis::RedisResponse resp{Redis::flush_pending(con)};


    for (int i{0}; i < 3; i++) {
        LOG_INFO("Output: {0}", resp.parse<std::string>());
    }
    //END - Pipeline test
 
    //START - Default test
    resp = Redis::execute(con, "SET", "name", "Max Mustermann");
    LOG_INFO(resp.parse<std::string>());
    resp = Redis::execute(con, "GET", "name");
    LOG_INFO(resp.parse<std::string>());


    resp = Redis::execute(con, "HELLO", "3");
    LOG_INFO(resp.parse<std::string>());
    
    //END - Default test
    */

   /*
    //START - Pub/Sub test

    Redis::RedisConnection con{ip_address, port};
    if (subscribe_to != "") {
        Redis::execute(con, "subscribe", subscribe_to);
        while (true) {
            LOG_INFO("Subscriber waiting for data");
            Redis::RedisResponse resp{con.getData()};
            LOG_INFO("Subscriber got data");
            LOG_INFO(resp.parse<std::string>());
        }
    }
    if (publish_to != "") {
        std::string input{};
        while (true) {
            std::cout << "Element: ";
            std::getline(std::cin, input);
            Redis::RedisResponse resp{Redis::execute(con, "publish", publish_to, input)};
            LOG_INFO(resp.parse<std::string>());
        }
    }
    //END - Pub/Sub test
    */
    
    /*
    Redis::RedisConnection con{ip_address, port};
    Redis::RedisLock lck{con, "lck_list"};

    lck.lock();
    
    std::string hello_output{Redis::execute(con, "hello", "3").parse<std::string>()};
    LOG_INFO(hello_output); 
    std::this_thread::sleep_for(std::chrono::milliseconds{2000});

    std::string set_output{Redis::execute(con, "set", "name", "hallowelt").parse<std::string>()};
    LOG_INFO(set_output);
    std::this_thread::sleep_for(std::chrono::milliseconds{2000});   

    std::string get_output{Redis::execute(con, "get", "name").parse<std::string>()};
    LOG_INFO(get_output);
    std::this_thread::sleep_for(std::chrono::milliseconds{2000});   

    lck.unlock();
    */

    Redis::RedisConnection con{ip_address, port};
    Redis::RedisTransaction transaction{con};
    std::string set_input{};
    std::string get_output{};

    set_input = Redis::execute(con, "set", "name", "Erster Name").parse<std::string>();
    LOG_INFO(set_input);
    
    transaction.begin_transaction();

    set_input = Redis::execute(con, "set", "name", "Zweiter Name").parse<std::string>();
    LOG_INFO(set_input);

    get_output = Redis::execute(con, "get", "name").parse<std::string>();
    LOG_INFO(get_output);

    transaction.end_transaction();

    //transaction.discard_transaction();

    get_output = Redis::execute(con, "get", "name").parse<std::string>();
    LOG_INFO(get_output);
    
    return 0;
}
