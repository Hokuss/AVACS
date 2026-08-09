#include "watcher.hpp"
#include "ast.hpp"
#include <chrono>
#include <filesystem>
#include <iostream>
#include <ostream>
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