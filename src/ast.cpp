#include "ast.hpp"
#include "utils.hpp"
#include <cctype>
#include <cstddef>
#include <iostream>
#include <memory>
#include <string_view>
#include <vector>

size_t cursor = 0;



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

token ast::next_token(){
    if(cursor >= source.size()) return {grammar::EF, 0 , ""};
    size_t start = cursor;
    char current = source[cursor];

    switch (source[cursor]){
        case '=': cursor++; return {grammar::EQUAL, 1, source.substr(start,1)};
        case '{': cursor++; return {grammar::OPEN_BRACE, 1, source.substr(start,1)};
        case '}': cursor++; return {grammar::CLOSE_BRACE, 1, source.substr(start, 1)};
        case '[': cursor++; return {grammar::OPEN_BRACKET, 1, source.substr(start, 1)};
        case ']': cursor++; return {grammar::CLOSE_BRACKET, 1, source.substr(start, 1)};
        case '+': cursor++; return {grammar::ADD, 1, source.substr(start,1)};
        case '-': cursor++; return {grammar::SUBTRACT, 1, source.substr(start, 1)};
        case '*': cursor++; return {grammar::MULTIPLY, 1, source.substr(start, 1)};
        case '/': cursor++; return {grammar::DIVIDE, 1, source.substr(start, 1)};
        case ';': cursor++; return {grammar::EOS, 1, source.substr(start, 1)};
        case '\n': cursor++; return {grammar::EOL, 1, source.substr(start, 1)};
    }

    if (current == ' ' || current == '\r' || current == '\t'){
        while (cursor < source.size() && (source[cursor] == ' ' || source[cursor] == '\t' || source[cursor] == '\r')) cursor++;

        return {grammar::WHITE_SPACE, cursor - start, source.substr(start, cursor-start)};
    }

    if (current == '\\') {
        cursor++; // Skip the leading backslash
        while (cursor < source.size() && (std::isalnum(source[cursor]) || source[cursor] == '\\' || source[cursor] == '_')) {
            cursor++;
        }
        return {grammar::PATH, cursor - start, source.substr(start, cursor - start)};
    }

    if (current == '"') {
        cursor++;
        while (cursor < source.size() && source[cursor] != '"') {
            cursor++;
        }
        if(cursor < source.size()) cursor++;
        return {grammar::STRING, cursor - start, source.substr(start, cursor - start)};
    }

    if (std::isalpha(current) || current == '_') {
        while (cursor < source.size() && (std::isalnum(source[cursor]) || source[cursor] == '_')) {
                cursor++;
        }
        std::string_view literal = source.substr(start, cursor - start);

        if (literal == "website") return {grammar::WEB, literal.size(), literal};
        if (literal == "data")    return {grammar::DATA, literal.size(), literal};
        if (literal == "logic")    return {grammar::LOGIC, literal.size(), literal};
        if (literal == "load") return {grammar::LOAD, literal.size(), literal};
        if (literal == "update") return {grammar::UPDATE, literal.size(), literal};
        if (literal == "assert") return {grammar::ASSERT, literal.size(), literal};
        if (literal == "include") return {grammar::INCLUDE, literal.size(), literal};

            
        return {grammar::IDENTIFIER, literal.size(), literal};
    }

    cursor++;
    return {grammar::EF, 1, source.substr(start, 1)};

}

token ast::peek(){
    size_t saved_cursor = cursor;
    token next = next_token();
    cursor = saved_cursor;
    return next;
}

std::shared_ptr<green_node> ast::next_leaf(){
    token temp = next_token();
    std::shared_ptr<green_node> leaf = std::make_shared<green_node>();
    leaf->size = temp.size;
    leaf->syntax = temp.type;
    leaf->look = temp.look;
    return leaf;
}


//Consume Blank spaces between blocks or functions
std::shared_ptr<green_node> ast::trivia_block(){
    //Create the trivia Block
    std::shared_ptr<green_node> triv = std::make_shared<green_node>();
    triv->syntax = grammar::TRIVIA;
    triv->size = 0; //0 for now

    //peek
    token peeker = peek();
    while (peeker.type==grammar::EOL || peeker.type==grammar::WHITE_SPACE ){
        std::shared_ptr<green_node> leaf = next_leaf();
        triv->child.push_back(leaf);
        triv->size += leaf->size;
        peeker = peek();
    }

    return triv;
}

// Identifiers
std::shared_ptr<green_node> ast::assignment_block() {
    std::shared_ptr<green_node> assign = std::make_shared<green_node>();
    assign->syntax = grammar::ASSIGNMENT;
    assign->size = 0;

    // Helper lambda to safely add a child and accumulate its size
    auto add_child = [&](std::shared_ptr<green_node> child_node) {
        if (child_node) {
            assign->child.push_back(child_node);
            assign->size += child_node->size;
        }
    };

    // Helper lambda: Checks token type, logs error if mismatched, consumes token if matched
    auto expect_and_add = [&](grammar expected, const std::string& err_msg = "") -> bool {
        if (peek().type != expected) {
            if (!err_msg.empty()) {
                std::cerr << err_msg << std::endl;
            } else {
                std::cerr << "Expected - " << grammar_name(expected)
                          << " Found - " << grammar_name(peek().type) << std::endl;
            }
            return false;
        }
        add_child(next_leaf());
        return true;
    };

    // 1. Consume IDENTIFIER (already verified before entering function)
    add_child(next_leaf());
    add_child(trivia_block());

    // 2. Expect '='
    if (!expect_and_add(grammar::EQUAL, "Nothing to assign")) return assign;
    add_child(trivia_block());

    // 3. Expect STRING value
    if (!expect_and_add(grammar::STRING, "No string value to attach")) return assign;
    add_child(trivia_block());

    // 4. Expect EOS (end of statement/line)
    if (!expect_and_add(grammar::EOS, "No end of line Character")) return assign;

    assign->look = assign->child[0]->look;

    return assign;
}

std::shared_ptr<green_node> ast::web_block() {
    std::shared_ptr<green_node> web = std::make_shared<green_node>();
    web->syntax = grammar::WEBSITE_BLOCK;
    web->size = 0;

    // Helper lambda to safely add a child and accumulate its size
    auto add_child = [&](std::shared_ptr<green_node> child_node) {
        if (child_node) {
            web->child.push_back(child_node);
            web->size += child_node->size;
        }
    };

    // Helper lambda: Checks token type, logs error if mismatched, consumes token if matched
    auto expect_and_add = [&](grammar expected, const std::string& err_msg = "") -> bool {
        if (peek().type != expected) {
            if (!err_msg.empty()) {
                std::cerr << err_msg << std::endl;
            } else {
                std::cerr << "Expected - " << grammar_name(expected)
                          << " Found - " << grammar_name(peek().type) << std::endl;
            }
            return false;
        }
        add_child(next_leaf());
        return true;
    };

    // 1. Consume 'WEB' token
    add_child(next_leaf());

    // 2. Mandatory whitespace check
    if (peek().type != grammar::WHITE_SPACE) {
        std::cerr << "Invalid Web Function: Expected whitespace" << std::endl;
        return web;
    }
    add_child(trivia_block());

    // 3. Expect PATH
    if (!expect_and_add(grammar::PATH, "Path Not detected")) return web;
    add_child(trivia_block());

    // 4. Expect '{'
    if (!expect_and_add(grammar::OPEN_BRACE, "Web Function Not Detected: Expected '{'")) return web;

    // 5. Body loop
    while (peek().type != grammar::CLOSE_BRACE && peek().type != grammar::EF) {
        add_child(trivia_block());

        grammar current = peek().type;
        if (current == grammar::CLOSE_BRACE || current == grammar::EF) {
            break;
        }

        if (current == grammar::IDENTIFIER) {
            add_child(assignment_block());
        } else {
            std::cerr << "Unexpected token in web block body: " << grammar_name(current) << std::endl;
            add_child(next_leaf()); // Advance token to prevent infinite loops
        }
    }

    // 6. Expect closing '}'
    expect_and_add(grammar::CLOSE_BRACE, "Expected '}' at end of web block");

    return web;
}

std::shared_ptr<green_node> ast::load_block(){
    std::shared_ptr<green_node> load = std::make_shared<green_node>();
    load->syntax = grammar::LOAD_BLOCK;
    load->size = 0;

    auto add_child = [&](std::shared_ptr<green_node> child_node) {
        if (child_node) {
            load->child.push_back(child_node);
            load->size += child_node->size;
        }
    };

    auto expect_and_add = [&](grammar expected, const std::string& err_msg = "") -> bool {
        if (peek().type != expected) {
            if (!err_msg.empty()) {
                std::cerr << err_msg << std::endl;
            } else {
                std::cerr << "Expected - " << grammar_name(expected)
                          << " Found - " << grammar_name(peek().type) << std::endl;
            }
            return false;
        }
        add_child(next_leaf());
        return true;
    };

    add_child(next_leaf());

    add_child(trivia_block());

    expect_and_add(grammar::OPEN_BRACE);
    add_child(trivia_block());
    expect_and_add(grammar::CLOSE_BRACE);

    return load;
}

std::shared_ptr<green_node> ast::update_block(){
    std::shared_ptr<green_node> updates = std::make_shared<green_node>();
    updates->syntax = grammar::UPDATE_BLOCK;
    updates->size = 0;

    auto add_child = [&](std::shared_ptr<green_node> child_node) {
        if (child_node) {
            updates->child.push_back(child_node);
            updates->size += child_node->size;
        }
    };

    auto expect_and_add = [&](grammar expected, const std::string& err_msg = "") -> bool {
        if (peek().type != expected) {
            if (!err_msg.empty()) {
                std::cerr << err_msg << std::endl;
            } else {
                std::cerr << "Expected - " << grammar_name(expected)
                          << " Found - " << grammar_name(peek().type) << std::endl;
            }
            return false;
        }
        add_child(next_leaf());
        return true;
    };

    add_child(next_leaf());
    add_child(trivia_block());

    expect_and_add(grammar::OPEN_BRACE);
    add_child(trivia_block());
    expect_and_add(grammar::CLOSE_BRACE);

    return updates;
}

/* Data Bloc time
remember to create/update/load/store/protected keywords
*/
std::shared_ptr<green_node> ast::data_block(){
    std::shared_ptr<green_node> data = std::make_shared<green_node>();
    data->syntax = grammar::DATA_BLOCK;
    data->size = 0;

    auto add_child = [&](std::shared_ptr<green_node> child_node) {
        if (child_node) {
            data->child.push_back(child_node);
            data->size += child_node->size;
        }
    };

    auto expect_and_add = [&](grammar expected, const std::string& err_msg = "") -> bool {
        if (peek().type != expected) {
            if (!err_msg.empty()) {
                std::cerr << err_msg << std::endl;
            } else {
                std::cerr << "Expected - " << grammar_name(expected)
                          << " Found - " << grammar_name(peek().type) << std::endl;
            }
            return false;
        }
        add_child(next_leaf());
        return true;
    };

    //Consume data token
    add_child(next_leaf());
    add_child(trivia_block());

    expect_and_add(grammar::PATH,"Table not detected");
    add_child(trivia_block());

    expect_and_add(grammar::OPEN_BRACE);
    add_child(trivia_block());

    while (peek().type != grammar::CLOSE_BRACE && peek().type != grammar::EF) {
        add_child(trivia_block());

        grammar current = peek().type;
        if (current == grammar::CLOSE_BRACE || current == grammar::EF) {
            break;
        }

        if (current == grammar::IDENTIFIER) {
            add_child(assignment_block());
        } else if (current == grammar::LOAD) {
            add_child(load_block());
        } else if (current == grammar::UPDATE) {
            add_child(update_block());
        } else {
            std::cerr << "Unexpected token in web block body: " << grammar_name(current) << std::endl;
            add_child(next_leaf()); // Advance token to prevent infinite loops
        }
    }

    expect_and_add(grammar::CLOSE_BRACE, "End of the block not found");

    return data;
}

std::shared_ptr<green_node> ast::include_block(){
    std::shared_ptr<green_node> include = std::make_shared<green_node> ();
    include->syntax = grammar::INCLUDE_BLOCK;
    include->size = 0;

    auto add_child = [&](std::shared_ptr<green_node> child_node) {
        if (child_node) {
            include->child.push_back(child_node);
            include->size += child_node->size;
        }
    };

    auto expect_and_add = [&](grammar expected, const std::string& err_msg = "") -> bool {
        if (peek().type != expected) {
            if (!err_msg.empty()) {
                std::cerr << err_msg << std::endl;
            } else {
                std::cerr << "Expected - " << grammar_name(expected)
                          << " Found - " << grammar_name(peek().type) << std::endl;
            }
            return false;
        }
        add_child(next_leaf());
        return true;
    };

    add_child(next_leaf());
    add_child(trivia_block());

    expect_and_add(grammar::OPEN_BRACE);
    expect_and_add(grammar::STRING);
    expect_and_add(grammar::CLOSE_BRACE);

    return include;
}

void ast::parser() {
    raw_source = read_file(source_file);
    source = raw_source;
    cursor = 0;

    root_green = std::make_shared<green_node>();

    root_green->syntax = grammar::SOF;
    root_green->size = 0;

    token current_token = peek();

    while (current_token.type!=grammar::EF){
        switch (current_token.type) {
            case grammar::WHITE_SPACE:
            case grammar::EOL:
                root_green->child.push_back(trivia_block());
                break;

            case grammar::WEB:
                root_green->child.push_back(web_block());
                break;

            case grammar::DATA:
                root_green->child.push_back(data_block());
                break;

            // case grammar::LOGIC:
            //     root_green->child.push_back(logic_block());
            //     break;

            case grammar::INCLUDE:
                root_green->child.push_back(include_block());
                break;

            default: {
                std::shared_ptr<green_node> error_leaf = next_leaf();
                root_green->child.push_back(error_leaf);
                break;
            }
        }
        current_token = peek();
    }
    if (current_token.type == grammar::EF) {
        root_green->child.push_back(next_leaf());
    }


}

inline std::string to_lower(std::string s) {
    for (auto& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

// prefix   = the box-drawing characters accumulated from ancestors
// is_last  = whether this node is the last child of its parent (affects the connector/branch used)
void ast_print_impl(const std::shared_ptr<green_node>& node,
                     const std::string& prefix,
                     bool is_last,
                     bool is_root) {
    if (!node) return;

    std::cout << prefix;
    if (!is_root) {
        std::cout << (is_last ? "`-- " : "|-- ");
    }
    std::cout << to_lower(grammar_name(node->syntax))
               << " (" << node->look << ")\n";

    std::string child_prefix = prefix;
    if (!is_root) {
        child_prefix += (is_last ? "    " : "|   ");
    }

    for (size_t i = 0; i < node->child.size(); ++i) {
        bool last_child = (i + 1 == node->child.size());
        ast_print_impl(node->child[i], child_prefix, last_child, false);
    }
}

void ast_print(std::shared_ptr<green_node> root) {
    ast_print_impl(root, "", true, true);
}
void ast::update() {
    parser();
    // ast_print(root_green);
}

void compiler_context::lexer_check(){
    for(const auto& [file_name, ptr]: files){
        observer_ptr<ast> temp = ptr.get();
        for(std::weak_ptr<green_node> a: temp->root_green->child){
            bool rule = false;
            for (assertions check: rules){
                if (check.parent == a.lock()->syntax) {
                    rule = true;
                    std::shared_ptr<green_node> temp = a.lock();
                    std::vector<bool> satisfied (check.child.size(), false);
                    for (int i = 0; i<check.child.size();i++){
                        for(const auto & tree: temp->child){
                            if (tree->syntax == check.child[i].type && (tree->look == check.child[i].value || check.child[i].value =="") && !satisfied[i]) {
                                satisfied[i] = true;
                            } else if (tree->syntax == check.child[i].type && (tree->look == check.child[i].value || check.child[i].value =="")) {
                                satisfied[i] = false;
                                break;
                            }
                        }
                    }

                    for(int i = 0; i<satisfied.size();i++){
                        if (!satisfied[i]) {
                            std::cerr<<"Rule not satified -"<<grammar_name(temp->syntax)<<"Following required - "<<grammar_name(check.child[i].type) << "and" << check.child[i].value<<std::endl;
                        }
                    }
                }
                if(rule) break; 
            }
        }

    }
}

red_node::red_node(std::shared_ptr<green_node> g, observer_ptr<red_node> p, int val_start) : green(g), parent(p), start(val_start) {
    int i = val_start;
    int slot = 0;
    for (auto it: green->child){
        child.push_back(std::make_unique<red_node>(it,this, i));

        if (it->syntax==grammar::WEBSITE_BLOCK || it->syntax==grammar::DATA_BLOCK) {
            int j = i;
            for (auto con: it->child) {
                if (con->syntax==grammar::PATH) {
                    symbol temp;
                    temp.syntax = con->syntax;
                    temp.name = con->look;
                    temp.offset = j;
                    temp.vm_slot = slot++;
                    break;
                }
                
                j+=con->size;
            }
        }
        i += it->size;
        // Add Assignment
    }
}