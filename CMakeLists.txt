# CMakeList.txt : CMake project for TheWheel, include source and define
# project specific logic here.
#
include(src/cmake/compile_shaders.cmake)
cmake_minimum_required (VERSION 3.28)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("TheWheel")

# Set to C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")

# Avoid backslash issues with VULKAN_SDK on Windows
file(TO_CMAKE_PATH "$ENV{VULKAN_SDK}" VULKAN_SDK_FIXED)

# Print for sanity
message(STATUS "Vulkan include dirs: ${Vulkan_INCLUDE_DIRS}")

# Find packages
find_package(Vulkan REQUIRED)
find_package(SDL3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)

find_program(SLANGC_EXECUTABLE NAMES slangc REQUIRED)
message(STATUS "Using Slang compiler: ${SLANGC_EXECUTABLE}")

# Add .cpp and .h files recursively
file(GLOB_RECURSE SRC_FILES "${CMAKE_SOURCE_DIR}/src/*.cpp" "${CMAKE_SOURCE_DIR}/include/*.h")
add_executable(TheWheel ${SRC_FILES})

# Link directories
target_include_directories(TheWheel PRIVATE 
"${CMAKE_SOURCE_DIR}/include"
"${Vulkan_INCLUDE_DIRS}"
)

# Vulkan compiler definitions
target_compile_definitions(TheWheel PRIVATE
    VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
    VULKAN_HPP_NO_STRUCT_CONSTRUCTORS=1
)

# Use precompiled header "pch.h"
target_precompile_headers(TheWheel PRIVATE ${CMAKE_SOURCE_DIR}/include/pch.h)

# Link libraries
target_link_libraries(TheWheel PRIVATE 
Vulkan::Vulkan
SDL3::SDL3
glm::glm-header-only
imgui::imgui
)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET TheWheel PROPERTY CXX_STANDARD 20)
endif()

set(OUT_DIR ${CMAKE_BINARY_DIR}/../../shaders)
file(MAKE_DIRECTORY ${OUT_DIR})

add_slang_shader_target(COMPILED_SHADER SOURCES ${CMAKE_SOURCE_DIR}/src/shaders/triangle.slang)
add_dependencies(TheWheel COMPILED_SHADER)

# TODO: Add tests and install targets if needed.
