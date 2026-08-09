#include "utils.hpp"
#include <iostream>
#include <string>
#include <fstream>

std::string read_file(fs::path pth){
    std::ifstream file(pth, std::ios::in | std::ios::binary);
    if (!file.is_open()){
        std::cout<<pth.string() <<" - Path not found"<<std::endl;
        return "";
    }

    // Read the stream directly using stream iterators
    return std::string((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
}