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
            if (*stream) { 
                std::cout << "Stream established!" << std::endl;
            } else { 
                std::cout << "No connection!" << std::endl; 
            } 
        }

        ~RedisPipe() {
            if (*stream) {
                stream->close();
                std::cout << "Stream closed!" << std::endl;
            }

            delete stream;
            stream = nullptr;
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
            }
            return *this;
        }
    };
}