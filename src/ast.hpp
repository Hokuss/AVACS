#pragma once

#include <cstdint>
#include <memory>
#include <string>
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

struct green_node {
    grammar syntax;
    uint64_t size = 0;
    std::vector<std::shared_ptr<green_node>> child;
};

struct red_node {
    std::shared_ptr<green_node> green;
    std::weak_ptr<red_node> parent;
    uint64_t start = 0;
};

class ast{
    private:
        void parser();
        std::shared_ptr<green_node> load_block();
        std::shared_ptr<green_node> update_block();
        std::shared_ptr<green_node> assert_block();
        std::shared_ptr<green_node> next_leaf();
        std::shared_ptr<green_node> web_block();
        std::shared_ptr<green_node> data_block();
        // std::shared_ptr<green_node> logic_block();
        std::shared_ptr<green_node> assignment_block();
        // std::shared_ptr<green_node> include_block();
        std::shared_ptr<green_node> trivia_block();

    public:
        std::string source_file;
        std::shared_ptr<green_node> root_green;


        ast(std::string a){
            this->source_file = a;
            parser();
        };

        void update();


        

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

    public: 
        std::unordered_map<std::string, std::unique_ptr<ast>> files;
        void update_file(const std::string& path) {
            if (files.find(path) == files.end()) {
                files[path] = std::make_unique<ast>(path);
            }

            files[path]->update();
        }
        
};

extern compiler_context wx_compiler;