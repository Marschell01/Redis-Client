
#include <iostream>
#include <thread>

#include "redis_client.h"

int main() {
    Redis::RedisClient client("localhost", "6379");
    client.set("Name", "MaxMuster");
    
    std::cout << client.get("Nahme") << "\n";

    std::cout << client.get("Name") << std::endl;
    return 0;
}
