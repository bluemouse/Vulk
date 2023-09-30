# Debug helper to print the variables and imported targets of a package.
function(find_package_verbose PKG_NAME)
  get_directory_property(_varsBefore VARIABLES)
  get_directory_property(_targetsBefore IMPORTED_TARGETS)

  find_package(${PKG_NAME} ${ARGN})

  get_directory_property(_vars VARIABLES)
  list(REMOVE_ITEM _vars _varsBefore ${_varsBefore} _targetsBefore )
  message(STATUS "${PKG_NAME}:")
  message(STATUS "  VARIABLES:")
  foreach(_var IN LISTS _vars)
      message(STATUS "    ${_var} = ${${_var}}")
  endforeach()

  get_directory_property(_targets IMPORTED_TARGETS)
  list(REMOVE_ITEM _targets _targetsBefore ${_targetsBefore})
  message(STATUS "  IMPORTED_TARGETS:")
  foreach(_target IN LISTS _targets)
      message(STATUS "    ${_target}")
  endforeach()
endfunction()
