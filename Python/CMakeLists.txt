set(_python_dir "${CMAKE_BINARY_DIR}/python")
file(MAKE_DIRECTORY ${_python_dir})
file(MAKE_DIRECTORY ${_python_dir}/acts)

set(_python_install_dir "python/acts")

add_subdirectory(Utilities)

# This create the 'ActsPythonBindings' module
add_subdirectory(Module)

# Now add the sub modules
add_subdirectory(Core)
#add_subdirectory(Examples)

