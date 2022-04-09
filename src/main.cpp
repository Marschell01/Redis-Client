#include <string>
#include <chrono>

#include "CLI11.hpp"
#include "redis_client.hpp"
#include "proxy.hpp"

int main(int argc, char* argv[]) {
    std::string ip_address{"localhost"};
    int port{6379};
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

    int proxy_port{12345};
    Redis::RedisClient client{"localhost", proxy_port};

    /*
    std::string output;

    output = client.execute("SET", "name", "MaxMuster123").parse<std::string>();
    LOG_INFO(output);

    output = client.execute("GET", "name").parse<std::string>();
    LOG_INFO(output);
    */
    
    std::string output;
    client.execute_no_flush("SET", "name", "MaxMuster321");
    client.execute_no_flush("GET", "name");
    std::vector<Redis::RedisResponse> responses{client.flush_pending()};

    for (Redis::RedisResponse& response : responses) {
        LOG_INFO(response.parse<std::string>());
    }
    

    return 0;
}
