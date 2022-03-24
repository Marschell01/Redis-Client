#include "redis_client.h"

using namespace Redis;

RedisClient::RedisClient(std::string ip_address, std::string port) {
    
    con = new RedisConnection(ip_address, port);
}

RedisClient::~RedisClient() {
    delete con;
    con = nullptr;
    
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

    output += std::to_string(words.size()) + "\r\n";
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
    
    std::string cmd{create_command(input + ' ')};
    con->sendData(cmd);
    std::vector<std::string> resp{con->getData()};

    switch (BaseReply::determin_type(resp.at(0)))
    {
    case ReplyType::bulk_string:
        LOG_INFO(resp.at(1));
        break;
    case ReplyType::integer:
        LOG_INFO(resp.at(0));
        break;
    case ReplyType::simple_string:
        LOG_INFO(resp.at(0));
        break;
    case ReplyType::error:
        LOG_ERROR(resp.at(0));
        break;
    case ReplyType::null:
        LOG_ERROR("No entry found!");
        break;
    case ReplyType::map:
        {
            std::unique_ptr<MapReply> map = ReplyParser<MapReply>::parse(resp);
            map.get()->print_content();
            
            break;
        }
    case ReplyType::array:
        {
            std::unique_ptr<ArrayReply> array = ReplyParser<ArrayReply>::parse(resp);
            array.get()->print_content();
            
            break;
        }
    default:
        break;
    }
}


bool RedisClient::login(std::string user, std::string pwd) {

    if (user == "") {
        LOG_INFO("Logging in using default user!");
    }
    std::string cmd{create_command("HELLO 3 " + user + " " + pwd + " ")};

    con->sendData(cmd);
    std::vector<std::string> resp = con->getData();
    LOG_DEBUG("Recieved vector size: {0}", resp.size());

    std::unique_ptr<MapReply> head_info = ReplyParser<MapReply>::parse(resp);
    head_info.get()->print_content();
    
    LOG_INFO("Logging successfull!");
    
    return true;
}