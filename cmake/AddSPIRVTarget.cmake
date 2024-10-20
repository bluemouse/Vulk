# Add a target to compile shaders to SPIR-V

function(add_SPIRV_target TARGET)
  find_package(Vulkan REQUIRED COMPONENTS glslangValidator)

  cmake_parse_arguments ("SHADER" "" "" "TARGET_ENV;SOURCES;OUTPUT_DIR" ${ARGN})

  file(GLOB SHADER_SOURCE_FILES ${SHADER_SOURCES})

  set(SPIRV_OUTPUT_FILES "")
  set(COMMANDS "")
  foreach(SHADER_FILE ${SHADER_SOURCE_FILES})
    get_filename_component(SHADER_NAME ${SHADER_FILE} NAME)
    set(SPIRV_FILE ${SHADER_OUTPUT_DIR}/${SHADER_NAME}.spv)
    list(APPEND SPIRV_OUTPUT_FILES ${SPIRV_FILE})
    list(APPEND COMMANDS COMMAND ${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE} ARGS --target-env ${SHADER_TARGET_ENV} --quiet ${SHADER_FILE} -o ${SPIRV_FILE})
  endforeach()

  add_custom_command(
    OUTPUT ${SHADER_OUTPUT_DIR}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADER_OUTPUT_DIR}
  )
  add_custom_command(
    OUTPUT ${SPIRV_OUTPUT_FILES}
    ${COMMANDS}
    WORKING_DIRECTORY ${SHADER_OUTPUT_DIR}
    DEPENDS ${SHADER_OUTPUT_DIR} ${SHADER_SOURCE_FILES}
    COMMENT "Translating GLSL shaders to SPIR-V shaders (target env: vulkan1.0)"
    VERBATIM
  )
  add_custom_target(${TARGET} DEPENDS ${SPIRV_OUTPUT_FILES})
endfunction()
