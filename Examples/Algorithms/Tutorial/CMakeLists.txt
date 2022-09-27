add_library(
  ActsExamplesTutorial SHARED
  src/UserAlgorithm.cpp)
target_include_directories(
    ActsExamplesTutorial
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(
    ActsExamplesTutorial
  PUBLIC
    ActsCore
    ActsExamplesFramework)

install(
  TARGETS ActsExamplesTutorial
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})