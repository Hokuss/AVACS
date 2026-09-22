#pragma once

#include <stop_token>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <cstdint>
#include <vector>

void loop(std::stop_token stop, std::string main);

enum class req_type{
        GET,
        PUT,
        POST,
        PATCH,
        DELETE,
        DITCH
    };

class request {
    private:
        std::string content;
        void execute();
        void reader();
        void parse_request();
        std::thread extra;
        req_type method;
        std::string_view path, version;
        std::unordered_map<std::string_view, std::string_view> headers;
        std::string_view body;
        std::string ans;
        std::string ct;
        void vm_loop();
        std::string wrap_http_response();

        std::string wrap_http_response(const std::vector<uint8_t>& body);

    public:
        request() 
            : extra(&request::reader, this) 
        {
        }
        int status_code = 500;
        std::string_view main;
        std::string process();
};