#include <string>
#include <chrono>

#include "CLI11.hpp"
#include "redis_client.hpp"
#include "proxy.hpp"

int main(int argc, char* argv[]) {
    std::string ip_address{"localhost"};
    int destination_port{6379};
    bool sub{false};
    bool pub{false};
    
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
    app.add_flag("--sub", sub, "set client to subscriber");
    app.add_flag("--pub", pub, "set client to publisher");

    //LOG_SET_LOGLEVEL(LOG_LEVEL_DEBUG);

    CLI11_PARSE(app, argc, argv); 

    Redis::RedisClient client{ip_address, destination_port};
    if (!client.is_connected()) {
        LOG_ERROR("Proxy unavailable!");
        return 1;
    }

    /*
    std::string output;
    output = client.execute("SET", "name", "MaxMuster123").parse<std::string>();
    LOG_INFO(output);

    output = client.execute("GET", "name").parse<std::string>();
    LOG_INFO(output);

    output = client.execute("HELLO", "3").parse<std::string>();
    LOG_INFO(output);
    */
    
    /*
    std::string output;
    client.execute_no_flush("SET", "name", "MaxMuster321");
    client.execute_no_flush("GET", "name");
    std::vector<Redis::RedisResponse> responses{client.flush_pending()};

    for (Redis::RedisResponse& response : responses) {
        LOG_INFO(response.parse<std::string>());
    }
    */

    /*
    std::string output;
    client.lock("resource_1");

    try {
        std::this_thread::sleep_for(std::chrono::milliseconds{5000});
        output = client.execute("SET", "name", "MaxMuster123").parse<std::string>();
        LOG_INFO(output);

        output = client.execute("GET", "name").parse<std::string>();
        LOG_INFO(output);
        std::this_thread::sleep_for(std::chrono::milliseconds{2000});
    } catch(std::invalid_argument& e) {
        LOG_ERROR(e.what());
    }

    client.unlock("resource_1");
    */
    /*
    std::string output;
    try {
        output = client.execute("SET", "name", "MaxMuster321").parse<std::string>();
        LOG_INFO(output);

        client.begin_transaction();

        output = client.execute("SET", "name", "MaxMuster123").parse<std::string>();
        LOG_INFO(output);

        output = client.execute("GET", "name").parse<std::string>();
        LOG_INFO(output);
    
        //client.end_transaction();
        client.discard_transaction();

        output = client.execute("GET", "name").parse<std::string>();
        LOG_INFO(output);
    } catch(std::invalid_argument& e) {
        LOG_ERROR(e.what());
    }
    */

    std::string output;
    if (sub) {
        client.subscribe("subscribe_object");
        while (true) {
            LOG_INFO("Subscriber waiting for data");
            
            std::vector<Redis::RedisResponse> responses{client.fetch_data()};

            for (Redis::RedisResponse& response : responses) {
                LOG_INFO(response.parse<std::string>());
            }
            
        }
    }
    
    if (pub) {
        std::string input{};
        while (true) {
            std::cout << "Element: ";
            std::getline(std::cin, input);
            output = client.execute("PUBLISH", "subscribe_object", input).parse<std::string>();
            LOG_INFO(output);
        }
    }
    return 0;
}
