# source files will be added later
add_library(
  ActsExperimental SHARED "")
target_include_directories(
  ActsExperimental
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    # for the generated version header
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_link_libraries(
  ActsExperimental
  PUBLIC ActsCore)

install(
  TARGETS ActsExperimental
  EXPORT ActsExperimentalTargets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(
  DIRECTORY include/Acts
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# target source files are added separately
add_subdirectory(src/Geometry)
