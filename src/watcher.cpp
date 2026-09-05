#include "watcher.hpp"
#include "ast.hpp"
#include "utils.hpp"
#include <chrono>
#include <filesystem>
#include <iostream>
#include <ostream>
#include <stop_token>
#include <string_view>
#include <thread>

namespace fs = std::filesystem;

namespace {
    fs::file_time_type last_updated;
    fs::path main_loc;

    bool replace = false;

    
}
compiler_context wx_compiler;



void compile(){
    std::cout<<"Producing the new Compilation" << std::endl;
    wx_compiler.update_file("asset/main.wx");
    std::cout<<"Success"<<std::endl;
}

void loop(std::stop_token stop, std::string main){
    main_loc = main;

    // std::cout<<main<<std::endl;

    while (!stop.stop_requested()){
        if(!fs::exists(main_loc)){
            std::cerr<<"Main File Not Located. Old Compilation Hope"<<std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        fs::file_time_type current_updated = fs::last_write_time(main_loc);
        if(current_updated!=last_updated){
            compile();
            last_updated = current_updated;

            std::cout<<current_updated<<std::endl;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void request::reader(){
    content = read_file("./webx/final.hex");
}

void request::parse_request(){
    enum class pos_type {
        METHOD, TYPE, PATH,
        HEADER,
        CONTENT,
        ER
    };
    pos_type current = pos_type::METHOD;
    observer_ptr<const char> sptr, eptr, end;

    sptr = main.data();
    eptr = main.data();
    end = main.data() + main.size();
    while (eptr<end && current!= pos_type::ER) {
        switch (current) {
            case pos_type::METHOD:
                if(*eptr==' '){
                    std::string_view cut = std::string_view(sptr, eptr-sptr);
                    sptr = eptr + 1;
                    // if(cut=="")
                }
                break;
            default:
                std::cerr<<"Not Defined"<<std::endl;
        };
        eptr++;
    }
}


std::string request::process(){
    parse_request();
    
    return "";
}