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
        RedisConnection(const std::string& ip_address, const int& port, const std::string& con_name="") {
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

        std::deque<std::string> get_string_data() {
            std::deque<std::string> reply{};
            std::string temp{};
            asio::streambuf buf;

            LOG_DEBUG("get_string_data::{0}: Before read", connection_name);

            asio::read_until(socket, buf, '\n');

            LOG_DEBUG("get_string_data::{0}: After read!", connection_name);
            LOG_DEBUG("get_string_data::{0}: Creating stream!", connection_name);

            std::istream is{&buf};

            LOG_DEBUG("get_string_data::{0}: Created stream!", connection_name);
            LOG_DEBUG("get_string_data::{0}: Processing received message!", connection_name);
            while (std::getline(is, temp)) {
                if (temp.back() == '\r') {
                    temp.pop_back();
                }
                reply.push_back(temp);
            }
            LOG_DEBUG("get_string_data::{0}: Processed received message!", connection_name);
            return reply;
        }

        MessageBundle get_proto_data() {
            MessageBundle messages = MessageBundle::default_instance();

            LOG_DEBUG("get_proto_data::{0}: GOT MESSAGE!", connection_name);
            u_int64_t response_size;
            asio::read(socket, asio::buffer(&response_size, sizeof(response_size)));
            asio::streambuf buf;
            asio::streambuf::mutable_buffers_type bufs = buf.prepare(response_size);
            buf.commit(asio::read(socket, bufs));

            std::istream is{&buf};
            messages.ParseFromIstream(&is);

            return messages;
        }

        void buffer_string_data(const std::string& request) {
            message_buffer.append(request);
        }

        void buffer_proto_data(const Message& request) {
            message_bundle.add_message()->MergeFrom(request);
        }

        void send_proto_data() {
            LOG_DEBUG("send_proto_data::{0}: Before write!", connection_name);

            u_int64_t request_size{message_bundle.ByteSizeLong()};
            asio::write(socket, asio::buffer(&request_size, sizeof(request_size)));

            asio::streambuf buf;
            std::ostream os{&buf};
            message_bundle.SerializeToOstream(&os);
            asio::write(socket, buf);
            message_bundle.clear_message();
            LOG_DEBUG("send_proto_data::{0}: Sent message!", connection_name);
        }

        void send_proto_with_mode(const std::string& mode) {
            message_bundle.set_mode(mode);
            LOG_DEBUG("send_proto_data:: {0}: Mode set to {1}", connection_name, mode);
            send_proto_data();
        }

        void send_string_data(const std::string& request) {
            LOG_DEBUG("send_string_data::{0}: Before write!", connection_name);
            asio::write(socket, asio::buffer(request, request.size()));
            LOG_DEBUG("send_string_data::{0}: Sent message!", connection_name);
        }
    };
}