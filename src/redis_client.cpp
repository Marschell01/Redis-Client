#include "redis_client.h"

using namespace Redis;

RedisClient::RedisClient(std::string ip_address, std::string port) {

    auto client_logger = spdlog::stderr_color_mt("client");
    pipe = new RedisPipe<std::string>(ip_address, port);
    if (!pipe->stream_open()) {
        client_logger->error("No connection to {0}, using port {1}, could be established!", ip_address, port);
    }
    client_logger->info("Connection to {0}, using port {1}, established!", ip_address, port);
}

RedisClient::~RedisClient() {
    delete pipe;
    pipe = nullptr;
}

std::string RedisClient::create_command(std::string input) {
    std::string output{"*"};

    char space_delimiter = ' ';
    std::vector<std::string> words{};

    size_t pos = 0;
    while ((pos = input.find(space_delimiter)) != std::string::npos) {
        std::string found{input.substr(0, pos)};
        if (!found.empty()) {
            words.push_back(found);
        }
        input.erase(0, pos + 1);
    }

    //header
    output += std::to_string(words.size()) + "\r\n";
    //content
    for (const auto &str : words) {
        output += "$" + std::to_string(str.length()) + "\r\n";
        output += str + "\r\n";
    }

    return output;
}

size_t RedisClient::get_length(std::string header) {
    size_t length{0};

    for (size_t i=1; i < header.length(); i++) {
        if (header[i] == '\r') {
            break;
        }
        length = (length * 10) + (header[i] - '0');
        
    }

    return length;
}

void RedisClient::execute(std::string input) {
    *pipe << create_command(input + ' ');
    std::string output;
    *pipe >> output;
    ReplyType replyType{Reply::determine_type(output)};
    if (!(replyType == ReplyType::array)) {
    } 
    std::cout << output << std::endl;
}


bool RedisClient::login(std::string user, std::string pwd) {
    auto login_logger = spdlog::stdout_color_mt("login");
    if (user == "") {
        login_logger->info("Logging in using default user!");
    }
    std::string cmd{"HELLO 3 " + user + " " + pwd + " "};
    std::string output{""};

    *pipe << create_command(cmd);
    *pipe >> output;
    if (output[0] == '-') {
        login_logger->error("Logging failed!");
        return false;
    }
    login_logger->info("Logging successfull!");

    size_t out_len = get_length(output);
    for (size_t i{0}; i < out_len; i++) {
        *pipe >> output;
        *pipe >> output;

        std::cout << output << " => ";
        
        *pipe >> output;
        if (output[0] == '$') {
            *pipe >> output;
        }
        std::cout << output << "\n";
    }
    std::cout << std::flush;

    return true;
}