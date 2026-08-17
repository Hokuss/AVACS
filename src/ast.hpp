#pragma once

#include "utils.hpp"
#include <array>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

enum class grammar {
    // Trivia / Tokens
    SOF, TRIVIA,
    WHITE_SPACE, EOL, IDENTIFIER, STRING, EOS,
    OPEN_BRACE, CLOSE_BRACE, EQUAL, OPEN_BRACKET, CLOSE_BRACKET, EF,
    ADD, SUBTRACT, MULTIPLY, DIVIDE,
    PATH, WEB, DATA, LOGIC, INCLUDE,
    LOAD, UPDATE, ASSERT,
    
    // Complex Structural Nodes
    WEBSITE_BLOCK, DATA_BLOCK, LOGIC_BLOCK, ASSIGNMENT, INCLUDE_BLOCK, LOAD_BLOCK, UPDATE_BLOCK, ASSERT_BLOCK
};

struct symbol{
    grammar syntax;
    std::string name;
    uint64_t offset = 0;
    uint64_t vm_slot = 0;
};

struct green_node {
    grammar syntax;
    uint64_t size = 0;
    std::string_view look;
    std::vector<std::shared_ptr<green_node>> child;
};

struct red_node {
    std::shared_ptr<green_node> green;
    std::vector<std::unique_ptr<red_node>> child;
    observer_ptr<red_node> parent;
    uint64_t start = 0;

    std::unordered_map<std::string_view, symbol> symbols;

    red_node(std::shared_ptr<green_node> g,int &vm_slot, observer_ptr<red_node> p = nullptr, int val_start = 0);
    red_node(observer_ptr<red_node> child);

    observer_ptr<symbol> resolve(std::string_view sym) {
        if(symbols.find(sym)!=symbols.end()){
            return &symbols[sym];
        } else if (parent!=nullptr) {
            return parent->resolve(sym);
        } 
        return nullptr;
    };
};

struct flat_tree{
    std::vector<observer_ptr<red_node>> child;
};

struct token {
    grammar type;
    uint64_t size;
    std::string_view look;
};

class ast{
    private:
        std::string raw_source;
        std::string_view source;
        token next_token();
        token peek();
        void parser();
        std::shared_ptr<green_node> load_block();
        std::shared_ptr<green_node> update_block();
        std::shared_ptr<green_node> assert_block();
        std::shared_ptr<green_node> next_leaf();
        std::shared_ptr<green_node> web_block();
        std::shared_ptr<green_node> data_block();
        // std::shared_ptr<green_node> logic_block();
        std::shared_ptr<green_node> assignment_block();
        std::shared_ptr<green_node> include_block();
        std::shared_ptr<green_node> trivia_block();
        int vm_slot = 0;

    public:
        std::string source_file;
        
        std::shared_ptr<green_node> root_green;
        std::shared_ptr<red_node> root_red;


        ast(std::string a){
            this->source_file = a;
            parser();
        };

        void update();
        void red_build();
        

        // std::shared_ptr<red_node> get_red_tree() {
        //     if (!root_green) return nullptr;
            
        //     auto red_root = std::make_shared<red_node>();
        //     red_root->green = root_green;
        //     red_root->start = 0;
        //     // parent is left empty (None) for the root
            
        //     return red_root;
        // }
};

class compiler_context {
    private:
        void lexer_check();
        std::string source = "./webx/out.hex";
        void full_ast(observer_ptr<red_node> tree_taversal, bool root = false);
        std::unique_ptr<flat_tree> tree;
        void compile();

    public: 
        std::unordered_map<std::string, std::unique_ptr<ast>> files;
        void update_file(const std::string& path) {
            if (files.find(path) == files.end()) {
                files[path] = std::make_unique<ast>(path);
            }

            files[path]->update();
            lexer_check();
            files[path]->red_build();
            compile();
            
        }
        
};

extern compiler_context wx_compiler;

/*
similar rules for finding the alphanum like web/data/include blocks as well
*/

struct child_rule {
    grammar type;
    std::string_view value;
};

struct assertions{
    grammar parent;
    std::span<const child_rule> child;
};

constexpr std::array<child_rule, 1> rinc = {{
    {grammar::STRING, ""}
}};

constexpr std::array<child_rule, 1> rweb = {{
    {grammar::ASSIGNMENT, "source"}
}};

constexpr std::array<assertions, 2> rules = {{
    {grammar::INCLUDE_BLOCK, rinc},
    {grammar::WEBSITE_BLOCK, rweb}
}};