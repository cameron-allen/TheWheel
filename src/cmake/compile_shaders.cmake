
# WIP
macro(create_spv TARGET)
    cmake_parse_arguments("SHADER" "VERT;FRAG;" "FILE_NAME" "" ${ARGN})
    set(STAGE "")

    if(SHADER_VERT)
        list(APPEND STAGE -stage vertex)
    elseif(SHADER_FRAG)
        list(APPEND STAGE -stage fragment)
    endif()

    add_custom_command (
        OUTPUT  ${OUT_DIR}/${SHADER_FILE_NAME}.spv
        COMMAND ${SLANGC_EXECUTABLE} ${SHADER_SOURCES} -target spirv -profile spirv_1_4 -emit-spirv-directly -fvk-use-entrypoint-name ${STAGE} ${ENTRY_POINTS} -o ${SHADER_FILE_NAME}.spv
        WORKING_DIRECTORY ${OUT_DIR}
        DEPENDS ${SHADER_SOURCES}
        COMMENT "Compiling Slang Shader File: ${SHADER_FILE_NAME}.spv"
        VERBATIM
    )
    add_custom_target(${TARGET} DEPENDS ${OUT_DIR}/${SHADER_FILE_NAME}.spv)

endmacro()

macro(add_slang_shader_target TARGET)
    cmake_parse_arguments("SHADER" "" "" "ENTRY_POINTS;SOURCES" ${ARGN})
    list(GET SHADER_SOURCES 0 FIRST_SOURCE)
    get_filename_component(FILE_NAME "${FIRST_SOURCE}" NAME_WE)

    set(ENTRY_POINTS "")
    if(NOT SHADER_ENTRY_POINTS)
        list(APPEND ENTRY_POINTS -entry vertMain -entry fragMain)
    elseif()
        foreach(e IN LISTS SHADER_ENTRY_POINTS)
            list(APPEND ENTRY_POINTS -entry "${e}")
        endforeach()
    endif()
  
    if(NOT SHADER_ENTRY_POINTS)
        create_spv(VERT_SPV FILE_NAME ${FILE_NAME}.vert)
        create_spv(FRAG_SPV FILE_NAME ${FILE_NAME}.frag)
        add_custom_target(${TARGET} DEPENDS VERT_SPV FRAG_SPV)
    else()
        create_spv(COMPILED_SPV FILE_NAME ${FILE_NAME})
        add_custom_target(${TARGET} DEPENDS COMPILED_SPV)
    endif()
endmacro()