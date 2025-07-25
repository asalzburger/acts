add_library(
    ActsExamplesDetectorTGeo
    SHARED
    src/AlignedTGeoDetectorElement.cpp
    src/TGeoDetector.cpp
    src/TGeoITkModuleSplitter.cpp
)

target_include_directories(
    ActsExamplesDetectorTGeo
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(
    ActsExamplesDetectorTGeo
    PUBLIC
        ActsCore
        ActsPluginRoot
        ActsPluginJson
        ActsExamplesFramework
        ActsExamplesDetectorGeneric
        ActsExamplesDetectorsCommon
        ActsExamplesITkModuleSplitting
)

acts_compile_headers(ActsExamplesDetectorTGeo GLOB "include/**/*.hpp")

install(
    TARGETS ActsExamplesDetectorTGeo
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY include/ActsExamples DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
