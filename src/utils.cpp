#include "utils.hpp"
#include <iostream>
#include <string>
#include <fstream>
#include <array>
#include <algorithm>

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

alignas(64) static constexpr auto char_lower_lut = [] {
    std::array<unsigned char, 256> table{};
    for (int i = 0; i < 256; ++i) {
        table[i] = (i >= 'A' && i <= 'Z') ? static_cast<unsigned char>(i + 32) : static_cast<unsigned char>(i);
    }
    return table;
}();

bool ilike_contains(std::string_view str, std::string_view pattern) noexcept {
    // Strip '%'
    if (!pattern.empty() && pattern.front() == '%') pattern.remove_prefix(1);
    if (!pattern.empty() && pattern.back() == '%') pattern.remove_suffix(1);

    const size_t p_len = pattern.size();
    const size_t s_len = str.size();

    if (p_len == 0) return true;
    if (p_len > s_len) return false;

    const auto* s = reinterpret_cast<const unsigned char*>(str.data());
    const auto* p = reinterpret_cast<const unsigned char*>(pattern.data());

    // Fast path: Single-character scan
    if (p_len == 1) {
        const unsigned char target = char_lower_lut[p[0]];
        for (size_t i = 0; i < s_len; ++i) {
            if (char_lower_lut[s[i]] == target) return true;
        }
        return false;
    }

    // Boyer-Moore-Horspool bad-character skip table
    size_t skip[256];
    std::fill_n(skip, 256, p_len);
    for (size_t i = 0; i < p_len - 1; ++i) {
        skip[char_lower_lut[p[i]]] = p_len - 1 - i;
    }

    const size_t last_idx = p_len - 1;
    const unsigned char last_char = char_lower_lut[p[last_idx]];
    size_t i = 0;

    // Sub-linear search loop
    while (i <= s_len - p_len) {
        const unsigned char c = char_lower_lut[s[i + last_idx]];
        if (c == last_char) {
            size_t j = last_idx;
            while (j > 0 && char_lower_lut[s[i + j - 1]] == char_lower_lut[p[j - 1]]) {
                --j;
            }
            if (j == 0) return true;
        }
        i += skip[c];
    }

    return false;
}