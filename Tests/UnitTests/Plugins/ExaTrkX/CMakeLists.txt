set(unittest_extra_libraries ActsPluginExaTrkX)

add_unittest(ExaTrkXBoostTrackBuilding ExaTrkXBoostTrackBuildingTests.cpp)
add_unittest(ExaTrkXMetricHookTests ExaTrkXMetricHookTests.cpp)
if(ACTS_EXATRKX_ENABLE_CUDA)
    add_unittest(ConnectedComponentsCuda ConnectedComponentCudaTests.cu)
    add_unittest(JunctionRemoval JunctionRemovalTests.cu)
endif()
add_unittest(Tensor TensorTests.cpp)
if(ACTS_EXATRKX_ENABLE_TORCH)
    set(unittest_extra_libraries ActsPluginExaTrkX ${TORCH_LIBRARIES})
    add_unittest(ExaTrkXTensorConversion ExaTrkXTensorConversionTests.cpp)
    add_unittest(ExaTrkXEdgeBuilding ExaTrkXEdgeBuildingTest.cpp)
endif()
