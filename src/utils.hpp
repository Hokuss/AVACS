#pragma once

#include <string>
#include <filesystem>

namespace fs = std::filesystem;

std::string read_file(fs::path pth);