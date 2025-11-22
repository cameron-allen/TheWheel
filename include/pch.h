#pragma once

// Global constant variables
constexpr bool DISPLAY_VULKAN_INFO = true;

// STL
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <fstream>
#include <unordered_map>

static std::vector<char> ReadFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("<ReadFile> failed to open file!");
    }

    std::vector<char> buffer(file.tellg());

    file.seekg(0, std::ios::beg);
    file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));

    file.close();
    
    return buffer;
}

