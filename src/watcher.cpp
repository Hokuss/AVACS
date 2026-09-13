#include "watcher.hpp"
#include "ast.hpp"
#include "utils.hpp"
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <map>
#include <filesystem>
#include <iostream>
#include <ostream>
#include <stop_token>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace fs = std::filesystem;

namespace {
    fs::file_time_type last_updated;
    fs::path main_loc;

    std::map<int, std::vector<uint8_t>> registers;

    /*
        Active operation and reversing it (may remove the activeregs later and directly use activenum as a pointer method)
        Immediate values are the reason, not doing it
    */
    std::vector<uint8_t> activeregs[4];
    int activenum[4];

    enum class op_code {
        //Jumping Instructions
        JMP,CJMP,

        //Register Loader
        LOAD,

        //File Interaction
        FILER, FILE,

        //Checking the Request
        PCH,

        //Final return Statement
        RET,

        //Reduntant
        ER
    };
    
    op_code current = op_code::ER;
    size_t pc = 0;
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

void request::vm_loop(){
    size_t i = 0, j = 0;
    int k = 0;

    auto helper = [](std::string_view a){
        if(a=="LOAD") return op_code::LOAD;
        else if (a=="RET") return op_code::RET;
        else if (a=="JMP") return op_code::JMP;
        else if (a=="CJMP") return op_code::CJMP;
        else if (a=="FILER") return op_code::RET;
        else if (a=="FILE") return op_code::FILE;
        else if (a=="PCH") return op_code::PCH;
        else return op_code::ER;
    };

    while(true && i<content.size()){
        if(current==op_code::ER && content[i]==' '){
            std::string_view temp = std::string_view(content).substr(j, i-j);
            current = helper(temp);
            j=i+1;
        } else if (content[i]==',' | content[i]=='\n') {
            if(content[j]=='R'){
                int regnum = std::stoi(content.substr(j+1, i-j));
                activenum[k] = regnum;
                activeregs[k++] = registers[regnum];
            } else if (content[j]=='"') {
                std::string_view temp = std::string_view(content).substr(j+1,i-j-1);
                activenum[k] = -1;
                activeregs[k++].assign(temp.begin(), temp.end());
            } else if (std::isdigit(content[j])) {
                activenum[k] = -1;
                std::string temp = content.substr(j, i-j);
                long long val = std::stoll(temp);
                activeregs[k].clear();
                do {
                    activeregs[k].push_back(static_cast<uint8_t>(val & 0xFF));
                    val >>= 8;
                } while (val>0);
                k++;
            }
            j=i+1;
            if(content[i]=='\n') {
                execute(); //execution code
                j = pc;
                i = pc-1;
            }
        } 
        i++;
    }
}

std::string request::process(){
    parse_request();
    if(extra.joinable()){
        extra.join();
        vm_loop();
    }
    return ans;
}

void request::execute(){
    switch (current) {
        case op_code::RET:
        {
            ans = std::string(reinterpret_cast<const char*>(activeregs[0].data()), activeregs[0].size());
            break;
        }
        case op_code::JMP:
        {
            pc = 0;
            for (size_t i = 0; i < activeregs[0].size(); ++i) {
                pc |= static_cast<uint32_t>(activeregs[0][i]) << (8 * i);
            }
            break;
        }
        case op_code::PCH:
        {
            std::string cmp = std::string(reinterpret_cast<const char*>(activeregs[0].data()), activeregs[0].size());
            if (path==cmp) {
                pc = 0;
                for (size_t i = 0; i < activeregs[1].size(); ++i) {
                    pc |= static_cast<uint32_t>(activeregs[1][i]) << (8 * i);
                }
            }
            break;
        }
        case op_code::LOAD:
        {
            registers[activenum[0]] = activeregs[1];
            break;
        }
        case op_code::FILER:
        {
            
        }
        case op_code::FILE:
        {

        }
        default:
            return;
    }
}