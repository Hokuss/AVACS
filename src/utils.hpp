#pragma once

#include <string>
#include <filesystem>

namespace fs = std::filesystem;
template <typename T>
using observer_ptr = T*;

std::string read_file(fs::path pth);