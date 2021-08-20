add_library(
  ActsExamplesGeant4 SHARED
  src/EventStoreRegistry.cpp
  src/GdmlDetectorConstruction.cpp
  src/Geant4Options.cpp  
  src/Geant4MaterialRecording.cpp
  src/MaterialEventAction.cpp
  src/MaterialRunAction.cpp
  src/MaterialSteppingAction.cpp
  src/MaterialGeneratorAction.cpp
  src/Geant4Simulation.cpp
  src/Geant4SurfaceMapper.cpp
  src/SimulationMagneticField.cpp
  src/SimulationGeneratorAction.cpp
  src/SimulationSteppingAction.cpp
  src/SimulationTrackingAction.cpp)
target_include_directories(
  ActsExamplesGeant4
  SYSTEM PUBLIC ${HEPMC3_INCLUDE_DIR})
target_include_directories(
    ActsExamplesGeant4
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(
  ActsExamplesGeant4
  PUBLIC ActsCore ActsExamplesFramework ${Geant4_LIBRARIES} Boost::program_options)

install(
  TARGETS ActsExamplesGeant4
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})