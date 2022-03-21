#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <asio.hpp>
#include <iostream>

namespace Redis {

    template <typename T>
    class RedisPipe {
    private:
        asio::ip::tcp::iostream* stream;

    public:

        RedisPipe(std::string ip_address, std::string port) {
            stream = new asio::ip::tcp::iostream(ip_address, port);
        }

        ~RedisPipe() {
            if (*stream) {
                stream->close();
                std::cout << "Stream closed!" << std::endl;
            }

            delete stream;
            stream = nullptr;
        }

        bool stream_open() {
            if (*stream) {
                return true;
            }
            return false;
        }

        RedisPipe& operator<<(T value) {
            if (*stream) {
                *stream << value;
            }
            return *this;
        }

        RedisPipe& operator>>(T& value) {
            if (*stream) {
                std::getline(*stream, value);
                if (value.back() == '\r') {
                    value.pop_back();
                }
            }
            return *this;
        }
    };
}