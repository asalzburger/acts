pybind11_add_module(ActsPythonBindings src/ActsPythonModule.cpp)

target_link_libraries(
    ActsPythonBindings
    PUBLIC
        ActsCore        
        ActsPythonUtilities
)

install(TARGETS ActsPythonBindings DESTINATION ${_python_install_dir})

set_target_properties(
    ActsPythonBindings
    PROPERTIES INSTALL_RPATH "\$ORIGIN/../../${CMAKE_INSTALL_LIBDIR}"
)
set_target_properties(
    ActsPythonBindings
    PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${_python_dir}/acts
)