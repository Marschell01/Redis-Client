#pragma once

#include <asio.hpp>
#include <iostream>
#include <string>
#include <thread>

#include "logger.h"
#include "redis.pb.h"

namespace Redis {

    class RedisProxy {
    private:
        static void serve_client(asio::ip::tcp::socket&& client_socket, asio::ip::tcp::socket&& proxy_socket) {
            std::string temp{};
            std::string request{""};
            std::string response{""};
            while (true) {
                response = "";
                request = "";
                try {
                    asio::streambuf request_buf;
                    asio::read_until(client_socket, request_buf, '\n');
                    std::istream request_is{&request_buf};
                    while (std::getline(request_is, temp)) {
                        if (temp.back() == '\r') {
                            temp.pop_back();
                        }
                        request.append(temp + "\r\n");
                    }
                    LOG_INFO("Got request");
                    LOG_DEBUG(request);

                    temp = "";
                    asio::write(proxy_socket, asio::buffer(request, request.size()));
                    asio::streambuf response_buffer;
                    asio::read_until(proxy_socket, response_buffer, '\n');
                    std::istream response_stream{&response_buffer};
                    while (std::getline(response_stream, temp)) {
                        if (temp.back() == '\r') {
                            temp.pop_back();
                        }
                        response.append(temp + "\r\n");
                    }
                    asio::write(client_socket, asio::buffer(response, response.size()));

                } catch (std::system_error& e) {
                    LOG_ERROR("Connection ended!");
                    client_socket.close();
                    return;
                }
            }
        }
    public:

        RedisProxy(int host_port, std::string dest_ip, int dest_port ) {

            LOG_INFO("Connecting to destination server");
            asio::io_context client_ctx;
            asio::ip::tcp::resolver resolver{client_ctx};
            auto results = resolver.resolve(dest_ip, std::to_string(dest_port));
            asio::ip::tcp::socket proxy_socket{client_ctx};
            asio::connect(proxy_socket, results);
            LOG_INFO("Connected to destination server!");


            LOG_INFO("Launching proxy server");
            asio::io_context server_ctx;
            asio::ip::tcp::endpoint ep{asio::ip::tcp::v4(), (unsigned short)host_port};
            asio::ip::tcp::acceptor acceptor{server_ctx, ep};

            acceptor.listen();
            LOG_INFO("Proxy launched");

            LOG_INFO("Waiting for connection!");
            asio::ip::tcp::socket client_socket{server_ctx};
            acceptor.accept(client_socket);
            LOG_INFO("Client connected to Proxy!");
                
            std::thread thd{serve_client, std::move(client_socket), std::move(proxy_socket)};
            thd.join();
            proxy_socket.close();
        }
    };
}