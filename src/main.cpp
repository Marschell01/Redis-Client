#include <string>
#include <chrono>

#include "CLI11.hpp"
#include "redis_client.hpp"

int main(int argc, char* argv[]) {
    std::string ip_address{"localhost"};
    std::string port{"6379"};
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

    //LOG_SET_LOGLEVEL(LOG_LEVEL_DEBUG);

    CLI11_PARSE(app, argc, argv); 

    Redis::RedisClient client{ip_address, port};
    client.lock();

    std::string output{client.execute("SET", "Name", "Max Musterman").parse<std::string>()};
    LOG_INFO(output);
    std::string igen{client.execute("GET", "Name").parse<std::string>()};
    LOG_INFO(igen);
    client.unlock();

    LOG_INFO(output);
    return 0;
}
