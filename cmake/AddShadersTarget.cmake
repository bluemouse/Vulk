# Add a target to compile shaders to SPIR-V

function(add_shaders_target TARGET)
  find_package(Vulkan REQUIRED COMPONENTS glslangValidator)

  cmake_parse_arguments ("SHADER" "" "" "SOURCES;OUTPUT_DIR" ${ARGN})
  file(GLOB SHADER_SOURCE_FILES ${SHADER_SOURCES})
  add_custom_command(
    OUTPUT ${SHADER_OUTPUT_DIR}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADER_OUTPUT_DIR}
  )
  add_custom_command(
    OUTPUT ${SHADER_OUTPUT_DIR}/frag.spv ${SHADER_OUTPUT_DIR}/vert.spv
    COMMAND ${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE}
    ARGS --target-env vulkan1.0 ${SHADER_SOURCE_FILES} --quiet
    WORKING_DIRECTORY ${SHADER_OUTPUT_DIR}
    DEPENDS ${SHADER_OUTPUT_DIR} ${SHADER_SOURCE_FILES}
    COMMENT "Compiling Shaders"
    VERBATIM
  )
  add_custom_target(${TARGET} DEPENDS ${SHADER_OUTPUT_DIR}/frag.spv ${SHADER_OUTPUT_DIR}/vert.spv)
endfunction()
