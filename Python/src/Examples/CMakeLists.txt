pybind11_add_module(ActsPythonBindingsExamples
  Framework.cpp
  ExamplesEntry.cpp
)

install(TARGETS ActsPythonBindingsExamples DESTINATION ${_python_install_dir})

set_target_properties(
    ActsPythonBindingsExamples
    PROPERTIES INSTALL_RPATH "\$ORIGIN/../../${CMAKE_INSTALL_LIBDIR}"
)
set_target_properties(
    ActsPythonBindingsExamples
    PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${_python_dir}/acts
)

target_link_libraries(
    ActsPythonBindingsExamples
    PUBLIC
        ActsCore
        ActsExamplesFramework
        ActsPythonUtilities
)