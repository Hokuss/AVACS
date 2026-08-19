#include "ast.hpp"
#include "utils.hpp"
#include <iostream>
#include <memory>
#include <ostream>

namespace {
    enum class opcode : uint8_t {
        LOAD_SLOT,
        STORE_SLOT,
        LOAD_PATH,
        CONNECT,
        LOAD_DATA,
        UPDATE_DATA
    };
    
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

void compiler_context::compile(){
    full_ast(files["asset/main.wx"]->root_red.get(), true);

    print_ast(tree.get());

    //Next step print
    //Start the bytecode generation/IR
    //Build the VM
}

