#pragma once

#include <asio.hpp>
#include <iostream>
#include <string>
#include <thread>
#include <regex>

#include "logger.h"
#include "redis_connection.hpp"
#include "redis.pb.h"

namespace Redis {

    class RedisProxy {
    private:
        static void serve_client(asio::ip::tcp::socket client_socket, asio::ip::tcp::socket proxy_socket) {
            RedisConnection client_connection{std::move(client_socket), "client"};
            RedisConnection server_connection{std::move(proxy_socket), "server"};
            bool sub_mode_on{false};
            bool first_launch{true};
            std::deque<std::string> server_response;
            MessageBundle msgs;
            std::string server_request{""};
            while (true) {
                try {
                    server_response.clear();
                    server_request.clear();
                    msgs = client_connection.get_proto_data();

                    if (msgs.mode() == "SUB") {
                        LOG_INFO("serve_client:: Set sub mode on");
                        sub_mode_on = true;
                    } else if (sub_mode_on && msgs.mode() != "ACK") {
                        LOG_ERROR("serve_client:: Subscribed client is not responding properly!");
                        return;
                    }

                    if (!sub_mode_on || first_launch) {
                        for (int i{0}; i < msgs.message_size(); i++) {
                            for (int y{0}; y < msgs.message(i).argument_size(); y++) {
                                server_request.append((msgs.message(i).argument(y)) + "\r\n");
                            }
                        }
                    }

                    LOG_INFO("serve_client:: Got from client!");

                    try {
                        if (!sub_mode_on || first_launch) {
                            server_connection.send_string_data(server_request);
                            LOG_INFO("SENT DATA");
                        }
                        server_response = std::deque<std::string>{server_connection.get_string_data()};
                    } catch(std::system_error& e) {
                        LOG_ERROR("serve_client:: Connection to server ended!");
                        return;
                    }

                    LOG_INFO("serve_client:: Got from server!");
                    Message msg;
                    for (const auto& e : server_response) {
                        LOG_DEBUG("serve_client:: argument: {0}", e);
                        msg.add_argument(e);
                    }
                    client_connection.buffer_proto_data(msg);

                    client_connection.send_proto_data();
                    LOG_INFO("serve_client:: Sent to client!");
                    
                    first_launch = false;
                } catch (std::system_error& e) {
                    client_socket.close();
                    LOG_ERROR("serve_client:: Connection to client ended!");
                    return;
                }
            }
        }
    public:

        RedisProxy(int host_port, std::string dest_ip, int dest_port ) {
            asio::io_context client_ctx;
            asio::ip::tcp::resolver resolver{client_ctx};
            asio::ip::tcp::socket proxy_socket{client_ctx};
            try {
                LOG_INFO("RedisProxy::Connecting to destination server");
                auto results = resolver.resolve(dest_ip, std::to_string(dest_port));
                asio::connect(proxy_socket, results);
                LOG_INFO("RedisProxy::Connected to destination server!");
            } catch (std::system_error& e) {
                LOG_ERROR("RedisProxy:: Could not connect to redis server");
                return;
            }

            try {
                LOG_INFO("RedisProxy::Launching Proxy");
                asio::io_context server_ctx;
                asio::ip::tcp::endpoint ep{asio::ip::tcp::v4(), (unsigned short)host_port};
                asio::ip::tcp::acceptor acceptor{server_ctx, ep};

                acceptor.listen();
                LOG_INFO("RedisProxy::Proxy launched");
                
                LOG_INFO("RedisProxy::Waiting for connection!");
                asio::ip::tcp::socket client_socket{server_ctx};
                acceptor.accept(client_socket);
                LOG_INFO("RedisProxy::Client connected to Proxy!");
                    
                std::thread thd{serve_client, std::move(client_socket), std::move(proxy_socket)};
                thd.join();
                proxy_socket.close();
            } catch(std::system_error& e) {
                LOG_ERROR("RedisProxy:: Error while handling client connection");
            }
            
            
        }
    };
}