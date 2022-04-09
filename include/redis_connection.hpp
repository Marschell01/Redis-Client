#pragma once

#include <asio.hpp>
#include <iostream>
#include <string>
#include <deque>

#include "logger.h"
#include "redis.pb.h"

namespace Redis {

    class RedisConnection {
    private:
        asio::io_context ctx;
        asio::ip::tcp::resolver resolver{ctx};
        asio::ip::tcp::socket socket{ctx}; 
        MessageBundle message_bundle{};
        std::string message_buffer;

    public:
        RedisConnection(std::string ip_address, int port) {
            LOG_INFO("Try to connect to server!");
                
            auto results = resolver.resolve(ip_address, std::to_string(port));
            asio::connect(socket, results);
                
            LOG_INFO("Connected to server!");
        }

        RedisConnection(asio::ip::tcp::socket sock) {
            socket = std::move(sock);
            LOG_INFO("Connected via RedisConnection!");
        }

        ~RedisConnection() {
            socket.close();
            LOG_INFO("Connection closed");
        }

        std::deque<std::string> getStringData() {
            std::deque<std::string> reply{};
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
                    if (temp.back() == '\r') {
                        temp.pop_back();
                    }
                    reply.push_back(temp);
                }

                LOG_DEBUG("Processed received message!");
                return reply;
            }
            LOG_ERROR("Can not get data, socket is closed!");
            return reply;
        }

        MessageBundle getProtoData() {
            MessageBundle messages = MessageBundle::default_instance();
            if (socket.is_open()) {
                LOG_DEBUG("GOT MESSAGE!");
                u_int64_t response_size;
                asio::read(socket, asio::buffer(&response_size, sizeof(response_size)));

                asio::streambuf buf;
                asio::streambuf::mutable_buffers_type bufs = buf.prepare(response_size);
                buf.commit(asio::read(socket, bufs));

                std::istream is{&buf};
                messages.ParseFromIstream(&is);

                return messages;
            }
            LOG_ERROR("Can not get data, socket is closed!");
            return messages;
        }

        void bufferStringData(const std::string& request) {
            message_buffer.append(request);
        }

        void bufferProtoData(const Message& request) {
            message_bundle.add_message()->MergeFrom(request);
        }

        void sendProtoData() {
            if (socket.is_open()) {
                LOG_DEBUG("Before write!");

                u_int64_t request_size{message_bundle.ByteSizeLong()};
                asio::write(socket, asio::buffer(&request_size, sizeof(request_size)));

                asio::streambuf buf;
                std::ostream os{&buf};
                message_bundle.SerializeToOstream(&os);
                asio::write(socket, buf);
                message_bundle.clear_message();
                LOG_DEBUG("Sent message!");
                return;
            }
            LOG_ERROR("Can not sent data, socket is closed!");
        }

        void sendStringData(std::string request) {
            if (socket.is_open()) {
                LOG_DEBUG("Before write!");

                asio::write(socket, asio::buffer(request, request.size()));

                LOG_DEBUG("Sent message!");
                return;
            }
            LOG_ERROR("Can not sent data, socket is closed!");
        }
    };
}