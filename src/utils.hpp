#pragma once

#include <string>
#include <filesystem>
#include <vector>

namespace fs = std::filesystem;
template <typename T>
using observer_ptr = T*;

std::string read_file(fs::path pth);
bool ilike_contains(std::string_view str, std::string_view pattern) noexcept;

std::string reverse_slash(std::string a);
std::ostream& operator<<(std::ostream& os, const std::vector<uint8_t>& vec);