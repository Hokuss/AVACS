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
        METHOD, 
        TYPE, 
        PATH,
        HEADER_KEY,
        HEADER_VALUE,
        BODY,
        ER
    };
    pos_type current = pos_type::METHOD;
    observer_ptr<const char> sptr, eptr, end;

    sptr = main.data();
    eptr = main.data();
    end = main.data() + main.size();
    std::string_view temp_key;
    while (eptr<end && current!= pos_type::ER) {
        switch (current) {
            case pos_type::METHOD:
                if(*eptr==' '){
                    std::string_view cut = std::string_view(sptr, eptr-sptr);
                    sptr = eptr + 1;
                    if(cut=="GET") method = req_type::GET;
                    else if (cut=="POST") method = req_type::POST;
                    else if (cut=="PUT") method = req_type::PUT;
                    else if (cut=="DELETE") method = req_type::DELETE;
                    else if (cut=="PATCH") method = req_type::PATCH;
                    else method = req_type::DITCH;

                    current = pos_type::PATH;
                }
                break;

            case pos_type::PATH:
                if(*eptr==' '){
                    path = std::string_view(sptr, eptr-sptr);
                    sptr = eptr+1;
                    current = pos_type::TYPE;
                }
                break;

            case pos_type::TYPE:
                if (eptr+1<end && *eptr=='\r' && *(eptr+1)=='\n') {
                    version = std::string_view(sptr, eptr-sptr);
                    sptr = eptr+1;
                    current = pos_type::HEADER_KEY;
                }
                break;

            case pos_type::HEADER_KEY:
                if (eptr+1<end && *eptr=='\r' && *(eptr+1)=='\n'){
                    eptr++;
                    sptr = eptr+1;
                    current = pos_type::BODY;
                } else if (*eptr==':') {
                    temp_key = std::string_view(sptr, eptr - sptr);
                    sptr = eptr + 1;
                    
                    // Skip leading whitespace after ':'
                    while (sptr < end && *sptr == ' ') {
                        sptr++;
                    }
                    eptr = sptr - 1; // Align scanner pointer
                    current = pos_type::HEADER_VALUE;
                }
                break;

            case pos_type::HEADER_VALUE:
                if (eptr + 1 < end && *eptr == '\r' && *(eptr + 1) == '\n') {
                    headers[temp_key] = std::string_view(sptr, eptr - sptr);
                    eptr++; // Consume '\r'
                    sptr = eptr + 1;
                    current = pos_type::HEADER_KEY;
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
    if(extra.joinable()){
        extra.join();
    }
    
    return "";
}