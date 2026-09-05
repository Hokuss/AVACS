#pragma once

#include <string>
#include <filesystem>

namespace fs = std::filesystem;
template <typename T>
using observer_ptr = T*;

std::string read_file(fs::path pth);
bool ilike_contains(std::string_view str, std::string_view pattern) noexcept;