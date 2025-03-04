add_library(ActsPythonUtilities INTERFACE)
target_include_directories(ActsPythonUtilities INTERFACE include)

set(_python_dir "${CMAKE_BINARY_DIR}/python")
file(MAKE_DIRECTORY ${_python_dir})
file(MAKE_DIRECTORY ${_python_dir}/acts)

set(_python_install_dir "python/acts")

# Core Python bindings
add_subdirectory(src/Core)

# Plugins Python bindings
add_subdirectory(src/Plugins)

# Examples Pythong bindings
add_subdirectory_if(src/Examples ACTS_BUILD_EXAMPLES)

target_link_libraries(ActsPythonBindings PUBLIC ActsPluginJson)
target_sources(ActsPythonBindings PRIVATE src/ModuleEntry.cpp)

set(py_files
    __init__.py
    examples/__init__.py
    examples/simulation.py
    examples/reconstruction.py
    examples/itk.py
    examples/odd.py
    _adapter.py
)

configure_file(setup.sh.in ${_python_dir}/setup.sh @ONLY)
install(FILES ${_python_dir}/setup.sh DESTINATION "python")

foreach(f ${py_files})
    set(_target ${_python_dir}/acts/${f})
    get_filename_component(_dir ${_target} DIRECTORY)
    file(MAKE_DIRECTORY ${_dir})

    file(
        CREATE_LINK ${CMAKE_CURRENT_SOURCE_DIR}/python/acts/${f} ${_target}
        SYMBOLIC
    )

    get_filename_component(_rel ${f} DIRECTORY)

    install(
        FILES ${CMAKE_CURRENT_SOURCE_DIR}/python/acts/${f}
        DESTINATION ${_python_install_dir}/${_rel}
    )
endforeach()
