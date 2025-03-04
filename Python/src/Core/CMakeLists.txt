pybind11_add_module(ActsPythonBindings
  Base.cpp
  CoreEntry.cpp
  Blueprint.cpp
  Definitions.cpp
  Geometry.cpp
  GeometryBuildingGen1.cpp
  Material.cpp
  Navigation.cpp
  Visualization.cpp
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

target_link_libraries(
    ActsPythonBindings
    PUBLIC
        ActsCore
        ActsPythonUtilities
)
