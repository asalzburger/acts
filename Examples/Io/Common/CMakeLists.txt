include(ActsTargetLinkLibrariesSystem)

add_library(
    ActsExamplesIoCommon
    SHARED
    src/SimHitHelper.cpp
)
target_include_directories(
    ActsExamplesIoCommon
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_link_libraries(
    ActsExamplesIoCommon
    PRIVATE
        ActsCore
        ActsExamplesFramework
)

install(TARGETS ActsExamplesIoCommon LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
