#pragma once

#include <asio.hpp>
#include <iostream>
#include <string>
#include <vector>
#include "logger.h"

namespace Redis {

    class RedisConnection {
    private:
        asio::io_context ctx;
        asio::ip::tcp::resolver resolver{ctx};
        asio::ip::tcp::socket socket{ctx}; 

    public:
        RedisConnection(std::string ip_address, std::string port) {
            LOG_INFO("Try to connect to server!");
            
            auto results = resolver.resolve(ip_address, port);
            asio::connect(socket, results);
            
            LOG_INFO("Connected to server!");
        }

        ~RedisConnection() {
            socket.close();
            LOG_INFO("Connection closed");
        }

        std::vector<std::string> getData() {
            std::vector<std::string> reply{};
            std::string temp{};

            if (socket.is_open()) {
                asio::streambuf buf;

                LOG_DEBUG("Before read");

                asio::read_until(socket, buf, '\n');

                LOG_DEBUG("After read!");
                LOG_DEBUG("Creating stream!");

                std::istream is{&buf};

                LOG_DEBUG("Created stream!");
                LOG_DEBUG("Processing received message!");
                while (std::getline(is, temp)) {
                    reply.push_back(temp);
                }

                LOG_DEBUG("Processed received message!");
                return reply;
            }
            LOG_ERROR("Can not get data, socket is closed!");
            return reply;
        }
                    

        void sendData(const std::string& message) {
            if (socket.is_open()) {
                LOG_DEBUG("Before write!");
                asio::write(socket, asio::buffer(message, message.size()));
                LOG_DEBUG("After write!");
                LOG_INFO("Sent message!");
                return;
            }
            LOG_ERROR("Can not sent data, socket is closed!");
        }
    };
}