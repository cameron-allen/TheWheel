# Define a C++23 modules library with:
#   NAME       – target name
#   MODULES    – list of .ixx / .cppm files (interface units)
#   SOURCES    – list of .cpp files (implementation units / normal sources)
function(add_module_library NAME)
    cmake_parse_arguments(MOD_LIB "" "" "MODULES;SOURCES" ${ARGN})

    # Require MODULES to be present
    if (NOT MOD_LIB_MODULES)
        message(FATAL_ERROR
            "add_module_library(${NAME}): the MODULES argument is required")
    endif()

    if (MOD_LIB_SOURCES)
        add_library(${NAME} STATIC ${MOD_LIB_SOURCES})
    else()
        add_library(${NAME} STATIC ${MOD_LIB_MODULES})
    endif()

    # Tell CMake which files are module interfaces
    target_sources(${NAME}
        PUBLIC
            FILE_SET cxx_modules TYPE CXX_MODULES FILES
                ${MOD_LIB_MODULES}
    )

endfunction()
