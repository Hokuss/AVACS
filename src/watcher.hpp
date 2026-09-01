#pragma once

#include <stop_token>
#include <string>
#include <string_view>

void loop(std::stop_token stop, std::string main);
void vm_loop(std::stop_token stop);

class request{
    std::string_view main;

    public:
        request(std::string req) {
            main = req;
        }

        
};