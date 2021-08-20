add_executable(
  ActsExampleMaterialRecordingDD4hep
  DD4hepMaterialRecording.cpp)
target_link_libraries(
  ActsExampleMaterialRecordingDD4hep
  PRIVATE
    ActsExamplesCommon
    ActsExamplesDetectorDD4hep
    ActsExamplesFramework
    ActsExamplesGeant4
    ActsExamplesGeant4DD4hep
    ActsExamplesIoRoot
    Boost::program_options)

install(
  TARGETS ActsExampleMaterialRecordingDD4hep
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_executable(
  ActsExampleSimulationDD4hep
  DD4hepSimulation.cpp)
target_link_libraries(
  ActsExampleSimulationDD4hep
  PRIVATE
    ActsExamplesCommon
    ActsExamplesFramework
    ActsExamplesDetectorDD4hep
    ActsExamplesGeant4
    ActsExamplesGeant4Common
    ActsExamplesGeant4DD4hep
    ActsExamplesMagneticField
    ActsExamplesIoRoot
    Boost::program_options)

install(
  TARGETS 
  ActsExampleSimulationDD4hep 
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
