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
        std::string connection_name;

    public:
        RedisConnection(std::string ip_address, int port, std::string con_name="") {
            LOG_INFO("RedisConnection::{0}: Try to connect to server!", con_name);
            
            auto results = resolver.resolve(ip_address, std::to_string(port));
            asio::connect(socket, results);
            connection_name = con_name;
                
            LOG_INFO("RedisConnection::{0}: Connected to server!", connection_name);
        }

        RedisConnection(asio::ip::tcp::socket sock, std::string con_name="") {
            socket = std::move(sock);
            connection_name = con_name;
            LOG_INFO("RedisConnection::{0}: Connected via RedisConnection!", connection_name);
        }

        ~RedisConnection() {
            socket.close();
            LOG_INFO("~RedisConnection::{0}: Connection closed", connection_name);
        }

        std::deque<std::string> getStringData() {
            std::deque<std::string> reply{};
            std::string temp{};

            if (socket.is_open()) {
                asio::streambuf buf;

                LOG_DEBUG("getStringData::{0}: Before read", connection_name);

                asio::read_until(socket, buf, '\n');

                LOG_DEBUG("getStringData::{0}: After read!", connection_name);
                LOG_DEBUG("getStringData::{0}: Creating stream!", connection_name);

                std::istream is{&buf};

                LOG_DEBUG("getStringData::{0}: Created stream!", connection_name);
                LOG_DEBUG("getStringData::{0}: Processing received message!", connection_name);
                while (std::getline(is, temp)) {
                    if (temp.back() == '\r') {
                        temp.pop_back();
                    }
                    reply.push_back(temp);
                }

                LOG_DEBUG("getStringData::{0}: Processed received message!", connection_name);
                return reply;
            }
            LOG_ERROR("getStringData::{0}: Can not get data, socket is closed!", connection_name);
            return reply;
        }

        MessageBundle getProtoData() {
            MessageBundle messages = MessageBundle::default_instance();
            if (socket.is_open()) {
                LOG_DEBUG("getProtoData::{0}: GOT MESSAGE!", connection_name);
                u_int64_t response_size;
                asio::read(socket, asio::buffer(&response_size, sizeof(response_size)));

                asio::streambuf buf;
                asio::streambuf::mutable_buffers_type bufs = buf.prepare(response_size);
                buf.commit(asio::read(socket, bufs));

                std::istream is{&buf};
                messages.ParseFromIstream(&is);

                return messages;
            }
            LOG_ERROR("getProtoData::{0}: Can not get data, socket is closed!", connection_name);
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
                LOG_DEBUG("sendProtoData::{0}: Before write!", connection_name);

                u_int64_t request_size{message_bundle.ByteSizeLong()};
                asio::write(socket, asio::buffer(&request_size, sizeof(request_size)));

                asio::streambuf buf;
                std::ostream os{&buf};
                message_bundle.SerializeToOstream(&os);
                asio::write(socket, buf);
                message_bundle.clear_message();
                LOG_DEBUG("sendProtoData::{0}: Sent message!", connection_name);
                return;
            }
            LOG_ERROR("sendProtoData::{0}: Can not sent data, socket is closed!", connection_name);
        }

        void sendStringData(std::string request) {
            if (socket.is_open()) {
                LOG_DEBUG("sendStringData::{0}: Before write!", connection_name);

                asio::write(socket, asio::buffer(request, request.size()));

                LOG_DEBUG("sendStringData::{0}: Sent message!", connection_name);
                return;
            }
            LOG_ERROR("sendStringData::{0}: Can not sent data, socket is closed!", connection_name);
        }
    };
}