#pragma once

#include <asio.hpp>
#include <iostream>
#include <string>
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
            try {
                auto results = resolver.resolve(ip_address, port);
                asio::connect(socket, results);
            } catch (asio::system_error& e) {
                LOG_ERROR(e.what());
            }
            LOG_INFO("Connected to server!");
        }

        ~RedisConnection() {
            socket.close();
            LOG_INFO("Connection closed");
        }

        std::string getData() {
            if (socket.is_open()) {
                asio::streambuf buf;
                LOG_DEBUG("Before read");
                asio::read_until(socket, buf, '\n');
                LOG_DEBUG("After read!");
                std::string reply;
                LOG_DEBUG("Creating stream!");
                std::istream is{&buf};
                LOG_DEBUG("Created stream!");

                LOG_DEBUG("Getting line!");
                getline(is, reply);
                LOG_DEBUG("Got line!");
                return reply;
            }
            LOG_ERROR("Can not get data, socket is closed!");
            return "";
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