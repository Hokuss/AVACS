#include "ast.hpp"
#include "utils.hpp"
#include <cstdint>
#include <fstream>
#include <iosfwd>
#include <iostream>
#include <memory>
#include <stack>
#include <string>
#include <unordered_map>
#include <vector>

namespace {
    uint64_t reg;
    int scope;
    std::stack<int> locals;
    
    std::unordered_map<std::string, int> pos;
    std::unordered_map<std::string, std::streampos> location;
    std::unordered_map<std::string, std::vector<uint64_t>> backprocess; 
}


inline const char* grammar_name(grammar g) {
    switch (g) {
        case grammar::SOF:               return "SOF";
        case grammar::TRIVIA:            return "TRIVIA";
        case grammar::WHITE_SPACE:       return "WHITE_SPACE";
        case grammar::EOL:               return "EOL";
        case grammar::IDENTIFIER:        return "IDENTIFIER";
        case grammar::STRING:            return "STRING";
        case grammar::EOS:               return "EOS";
        case grammar::OPEN_BRACE:        return "OPEN_BRACE";
        case grammar::CLOSE_BRACE:       return "CLOSE_BRACE";
        case grammar::EQUAL:             return "EQUAL";
        case grammar::OPEN_BRACKET:      return "OPEN_BRACKET";
        case grammar::CLOSE_BRACKET:     return "CLOSE_BRACKET";
        case grammar::EF:                return "EF";
        case grammar::ADD:               return "ADD";
        case grammar::SUBTRACT:          return "SUBTRACT";
        case grammar::MULTIPLY:          return "MULTIPLY";
        case grammar::DIVIDE:            return "DIVIDE";
        case grammar::PATH:              return "PATH";
        case grammar::WEB:               return "WEB";
        case grammar::DATA:              return "DATA";
        case grammar::LOGIC:             return "LOGIC";
        case grammar::LOAD:              return "LOAD";
        case grammar::ASSERT:            return "ASSERT";
        case grammar::UPDATE:            return "UPDATE";
        case grammar::INCLUDE:           return "INCLUDE";
        case grammar::WEBSITE_BLOCK:     return "WEBSITE_BLOCK";
        case grammar::DATA_BLOCK:        return "DATA_BLOCK";
        case grammar::LOGIC_BLOCK:       return "LOGIC_BLOCK";
        case grammar::ASSIGNMENT:        return "ASSIGNMENT";
        case grammar::INCLUDE_BLOCK:     return "INCLUDE_BLOCK";
        case grammar::LOAD_BLOCK:        return "LOAD_BLOCK";
        case grammar::UPDATE_BLOCK:      return "UPDATE_BLOCK";
        case grammar::ASSERT_BLOCK:      return "ASSERT_BLOCK";
        default:                         return "UNKNOWN";
    }
}

void pop_scope(){
    reg = locals.top();
    locals.pop();
    scope--;
}

void add_scope(){
    scope++;
    locals.push(reg);
}


void compiler_context::full_ast(observer_ptr<red_node> tree_taversal, bool root){
    if(root) tree = std::make_unique<flat_tree>();
    if(!tree_taversal) return;
    for(const auto& it: tree_taversal->child){
        if (it->green->syntax==grammar::INCLUDE_BLOCK) {
            for (auto includes: it->green->child) {
                if(includes->syntax==grammar::STRING && files.find(std::string(includes->look))!=files.end()){
                    full_ast(files[std::string(includes->look)]->root_red.get());
                }
            }
        }
        else if (it->green->syntax!=grammar::EF) {
            tree->child.push_back(it.get());
        }
    }
}

void print_ast(observer_ptr<flat_tree> a){
    for(auto it: a->child){
        ast_print(it, "", false, false);
    }
}

std::string resolve_load(std::shared_ptr<green_node> loader){
    std::string ans;
    if(loader->child[4]->syntax==grammar::STRING){
        add_scope();
        ans = "LOAD R"+std::to_string(reg++)+","+std::string(loader->child[4]->look)+"\n";
        ans += "FILER R"+std::to_string(reg-1) + "\n";
        pop_scope();
        return ans;
    }
    ans = "FILE R"+(std::to_string(reg++))+", R"+std::to_string(pos[std::string(loader->child[4]->look)])+"\n";
    return ans;
}

std::string resolve_assignment(std::shared_ptr<green_node> a){
    auto it = a->child[4];
    if(it->syntax==grammar::STRING) {
        return "LOAD R" + std::to_string(reg++) + "," + std::string(it->look)+"\n";
    }
    else if (it->syntax==grammar::LOAD_BLOCK){
        pos[std::string(a->child[0]->look)] = reg;
        return resolve_load(it);
    }
    return "";
}

void compiler_context::bytecode(){
    std::ofstream bytes(source,std::ios::binary | std::ios::trunc);
    if(!bytes.is_open()) std::cerr<<"JMP 0xFFFFFF\n";
    bytes<<"JMP 0xFFFFFF\n";
    reg = 0;
    for(observer_ptr<red_node> it: tree->child){
        if(it->green->syntax==grammar::WEBSITE_BLOCK){
            add_scope();
            for(auto per: it-> green -> child){
                switch (per->syntax) {
                    case grammar::PATH: location[std::string(per->look)] = bytes.tellp();
                        break;
                    case grammar::ASSIGNMENT: 
                        bytes<<resolve_assignment(per);
                        break;
                    case grammar::LOAD_BLOCK:
                        bytes<<resolve_load(per);
                        break;
                    default: 
                        break;
                        
                }
            }

        }
    }
    return;
}

void compiler_context::compile(){
    full_ast(files["asset/main.wx"]->root_red.get(), true);
    // print_ast(tree.get());
    bytecode();

    //Start the bytecode generation/IR
    //Build the VM
}

