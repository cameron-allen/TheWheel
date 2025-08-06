# CMakeList.txt : CMake project for TheWheel, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("TheWheel")

# Add external libraries
find_package(SDL3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)

#add_subdirectory(external/glm)

# Add .cpp and .h files recursively
file(GLOB_RECURSE SRC_FILES "${CMAKE_SOURCE_DIR}/src/*.cpp" "${CMAKE_SOURCE_DIR}/include/*.h")
add_executable(TheWheel ${SRC_FILES})

# Link directories
target_include_directories(TheWheel PRIVATE "${CMAKE_SOURCE_DIR}/include")

# Link libraries
target_link_libraries(TheWheel PRIVATE 
SDL3::SDL3
glm::glm-header-only
imgui::imgui
)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET TheWheel PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.
