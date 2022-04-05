#include <string>
#include <chrono>

#include "CLI11.hpp"
#include "proxy.hpp"

int main() {
    int dest_port{6379};
    int proxy_port{12345};
    Redis::RedisProxy proxy{proxy_port, "localhost", dest_port};

    return 0;
}
