add_library(
  ActsExamplesDetectorNNbar SHARED
  src/NNbarDetector.cpp
  )
target_include_directories(
    ActsExamplesDetectorNNbar
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(
    ActsExamplesDetectorNNbar
  PUBLIC ActsCore ActsExamplesFramework ActsExamplesGeant4 ActsPluginGeant4)

install(
  TARGETS ActsExamplesDetectorNNbar
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
  