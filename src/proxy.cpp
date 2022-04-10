/*
author: Dinhof Marcel
matnr: i17044
file: proxy.cpp
desc: This file implements the proxy launch and interface
date: 2022-04-09
class: 5b
catnr: 3
*/

#include "CLI11.hpp"
#include "proxy.hpp"

#include <string>
#include <chrono>

int main(int argc, char* argv[]) {
    int destination_port{6379};
    int host_port{12345};
    
    CLI::App app("A simple proxy for a redis client");
    app.add_option("--dport", destination_port, "port to send clients data to")->required();
    app.add_option("--hport", host_port, "the port on which the proxy should listen to for client connection")->required();

    //LOG_SET_LOGLEVEL(LOG_LEVEL_DEBUG);

    CLI11_PARSE(app, argc, argv); 

    Redis::RedisProxy proxy{host_port, "localhost", destination_port};

    return 0;
}
