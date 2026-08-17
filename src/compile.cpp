#include "ast.hpp"
#include "utils.hpp"
#include <memory>

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

void compiler_context::full_ast(observer_ptr<red_node> tree_taversal, bool root){
    if(root) tree = std::make_unique<flat_tree>();
    for(const auto& it: tree_taversal->child){
        if (it->green->syntax==grammar::INCLUDE_BLOCK) {
            for (auto includes: it->green->child) {
                if(includes->syntax==grammar::STRING){
                    full_ast(files[std::string(includes->look)]->root_red.get());
                }
            }
        }
        else if (it->green->syntax!=grammar::EF) {
            tree->child.push_back(it.get());
        }
    }

}

void compiler_context::compile(){
    full_ast(files["main.wx"]->root_red.get(), true);
}

