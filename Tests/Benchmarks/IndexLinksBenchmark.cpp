// This file is part of the Acts project.
//
// Copyright (C) 2017-2018 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "Acts/Definitions/Common.hpp"
#include "Acts/Experimental/IndexLinksImpl.hpp"
#include "Acts/Tests/CommonHelpers/BenchmarkTools.hpp"
#include "Acts/Utilities/detail/Axis.hpp"
#include "Acts/Utilities/detail/Grid.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <functional>
#include <set>
#include <vector>

#include <boost/container/small_vector.hpp>

using namespace Acts;
using namespace Acts::detail;

using EquidistantAxisClosed =
    Axis<AxisType::Equidistant, AxisBoundaryType::Closed>;
using EquidistantAxisBound =
    Axis<AxisType::Equidistant, AxisBoundaryType::Bound>;

/// Helper method to run index grid access benchmark on 1D grids
///
/// @tparam grid_impl_t the type of the index grid implementation
///
/// @param grid is the index grid implementation
/// @param kDim0 is the dimension, which is also the number of accesses
///
template <typename grid_impl_t>
void run_bench_1D(const grid_impl_t& grid, const unsigned int kDim0) {
  const ActsScalar kStep = 1. / ActsScalar(kDim0);
  Vector3 kVector(0., 0., 0);
  for (unsigned int k = 0; k < kDim0; ++k) {
    kVector[0] = (k + 0.5) * kStep;
    grid.links(kVector);
  }
}

/// Helper method to run index grid access benchmark on 2D grids
///
/// @tparam grid_impl_t the type of the index grid implementation
/// @tparam filler_t the type of the filler for the neighborhood access
///
/// @param grid is the index grid implementation
/// @param kDim0 is the dimension in one direction
/// @param kDim1 is the dimension in the other direction
///
template <typename grid_impl_t, typename filler_t>
void run_bench_2D(const grid_impl_t& grid, const unsigned int kDim0,
                  const unsigned int kDim1) {
  const ActsScalar zStep = 1. / ActsScalar(kDim0);
  const ActsScalar tStep = 1. / ActsScalar(kDim0);
  Vector3 kVector(0., 0., 0.);
  for (unsigned int iz = 0; iz < kDim0; ++iz) {
    kVector[2] = (iz + 0.5) * zStep;
    for (int it = -kDim1; it < static_cast<int>(kDim1); ++(++it)) {
      kVector[0] = (it + 0.5) * tStep;
      kVector[1] = (it + 0.5) * tStep;
      grid.template links<filler_t>(kVector);
    }
  }
}

/// Helper method to run index grid access benchmark on 2D grids
///
/// @tparam grid_impl_t the type of the index grid implementation
/// @tparam filler_t the type of the filler for the neighborhood access
///
/// @param grid is the index grid implementation
/// @param kDim0 is the dimension in one direction
/// @param kDim1 is the dimension in the other direction
///
template <typename grid_impl_t>
void run_bench_2D_bin_access(const grid_impl_t& grid, const unsigned int kDim0,
                             const unsigned int kDim1) {
  const ActsScalar zStep = 1. / ActsScalar(kDim0);
  const ActsScalar tStep = 1. / ActsScalar(kDim0);
  Vector3 kVector(0., 0., 0.);
  for (unsigned int iz = 0; iz < kDim0; ++iz) {
    kVector[2] = (iz + 0.5) * zStep;
    for (int it = -kDim1; it < static_cast<int>(kDim1); ++(++it)) {
      kVector[0] = (it + 0.5) * tStep;
      kVector[1] = (it + 0.5) * tStep;
      grid.links(kVector);
    }
  }
}

/// Main executable that runs the benchmark for index based grid access
///
/// There is the possibility to run 1D or 2D test
int main(int argc, char** argv) {
  unsigned int test = argc > 1 ? std::atoi(argv[1]) : 0;

  if (argc < 6 or test == 0 or test > 2) {
    std::cout << "*** Wrong parameters, please run with: " << std::endl;
    std::cout << "   <type of test: 1/2> <dim0> <dim1> <iterations> <runs>"
              << std::endl;
    return -1;
  }

  unsigned int kDim0 = std::atoi(argv[2]);
  unsigned int kDim1 = std::atoi(argv[3]);
  unsigned int kIterations = std::atoi(argv[4]);
  unsigned int kRuns = std::atoi(argv[5]);

  /// In these tests we always assume a return container either
  /// a) std::vector<> with duplicate removal
  /// b) small_vector<> with duplicate removal
  /// c) set
  /// as this is what is expected in the navigation, the number
  /// of candidate surfaces is not known at start

  using small_vector = boost::container::small_vector<unsigned int, 10>;

  /// Emulate a few scenarios
  /// (a) alow multiplicity gird with 10 elements - without neighbors
  /// Equidistant axis
  EquidistantAxisBound eAxis(0., kDim0 * 1., kDim0);
  Grid<unsigned int, decltype(eAxis)> eGrid1D(eAxis);
  Grid<std::array<unsigned int, 1>, decltype(eAxis)> aGrid1D(eAxis);
  Grid<std::set<unsigned int>, decltype(eAxis)> sGrid1D(eAxis);
  Grid<std::vector<unsigned int>, decltype(eAxis)> vGrid1D(eAxis);
  Grid<small_vector, decltype(eAxis)> svGrid1D(eAxis);

  // Let us fill the grid
  for (unsigned int ie = 0; ie < kDim0; ++ie) {
    eGrid1D.at(ie + 1u) = ie;
    aGrid1D.at(ie + 1u) = {ie};
    sGrid1D.at(ie + 1u) = {ie};
    vGrid1D.at(ie + 1u) = {ie};
    svGrid1D.at(ie + 1u) = {ie};
  }

  GridEntryImpl<decltype(eGrid1D), std::vector<unsigned int>> eToV1D(
      eGrid1D, {binX}, Transform3::Identity());

  GridEntryImpl<decltype(eGrid1D), std::set<unsigned int>> eToS1D(
      eGrid1D, {binX}, Transform3::Identity());

  GridEntryImpl<decltype(aGrid1D), std::vector<unsigned int>> aToV1D(
      aGrid1D, {binX}, Transform3::Identity());

  GridEntryImpl<decltype(aGrid1D), std::set<unsigned int>> aToS1D(
      aGrid1D, {binX}, Transform3::Identity());

  GridEntryImpl<decltype(sGrid1D), std::vector<unsigned int>> sToV1D(
      sGrid1D, {binX}, Transform3::Identity());

  GridEntryImpl<decltype(sGrid1D), std::set<unsigned int>> sToS1D(
      sGrid1D, {binX}, Transform3::Identity());

  GridEntryImpl<decltype(vGrid1D), std::vector<unsigned int>> vToV1D(
      vGrid1D, {binX}, Transform3::Identity());

  GridEntryImpl<decltype(vGrid1D), std::set<unsigned int>> vToS1D(
      vGrid1D, {binX}, Transform3::Identity());

  GridEntryImpl<decltype(svGrid1D), small_vector> svToSV1D(
      svGrid1D, {binX}, Transform3::Identity());

  GridEntryImpl<decltype(sGrid1D), small_vector> sToSV1D(
      sGrid1D, {binX}, Transform3::Identity());

  // Run the benchmarks
  if (test == 1) {
    std::cout << "*** 1D TEST SUITE *************** " << std::endl;
    std::cout << "***" << std::endl;
    std::cout << "*** Total number of accesses are "
              << kDim0 * kRuns * kIterations << std::endl;
    std::cout << "*** Test: " << kDim0 << " bins w/o neighborhood search "
              << std::endl;

    const auto eToV1D_result = Acts::Test::microBenchmark(
        [&] { run_bench_1D(eToV1D, kDim0); }, kIterations, kRuns);
    std::cout << " entry  -> vector : " << eToV1D_result << std::endl;

    const auto eToS1D_result = Acts::Test::microBenchmark(
        [&] { run_bench_1D(eToS1D, kDim0); }, kIterations, kRuns);
    std::cout << " entry  -> set    : " << eToV1D_result << std::endl;

    const auto aToV1D_result = Acts::Test::microBenchmark(
        [&] { run_bench_1D(aToV1D, kDim0); }, kIterations, kRuns);
    std::cout << " array  -> vector : " << aToV1D_result << std::endl;

    const auto aToS1D_result = Acts::Test::microBenchmark(
        [&] { run_bench_1D(aToS1D, kDim0); }, kIterations, kRuns);
    std::cout << " array  -> set    : " << aToS1D_result << std::endl;

    const auto vToV1D_result = Acts::Test::microBenchmark(
        [&] { run_bench_1D(vToV1D, kDim0); }, kIterations, kRuns);
    std::cout << " vector -> vector : " << vToV1D_result << std::endl;

    const auto vToS1D_result = Acts::Test::microBenchmark(
        [&] { run_bench_1D(vToS1D, kDim0); }, kIterations, kRuns);
    std::cout << " vector -> set    : " << vToS1D_result << std::endl;

    const auto sToV1D_result = Acts::Test::microBenchmark(
        [&] { run_bench_1D(sToV1D, kDim0); }, kIterations, kRuns);
    std::cout << " set    -> vector : " << sToV1D_result << std::endl;

    const auto sToS1D_result = Acts::Test::microBenchmark(
        [&] { run_bench_1D(sToS1D, kDim0); }, kIterations, kRuns);
    std::cout << " set    -> set    : " << sToS1D_result << std::endl;

    const auto svToSV1D_result = Acts::Test::microBenchmark(
        [&] { run_bench_1D(svToSV1D, kDim0); }, kIterations, kRuns);
    std::cout << " svect  -> svect  : " << svToSV1D_result << std::endl;

    const auto sToSV1D_result = Acts::Test::microBenchmark(
        [&] { run_bench_1D(sToSV1D, kDim0); }, kIterations, kRuns);
    std::cout << " set    -> svect  : " << sToSV1D_result << std::endl;

  } else if (test == 2) {
    std::cout << "*** 2D TEST SUITE *************** " << std::endl;
    std::cout << "***" << std::endl;
    std::cout << "*** Total number of accesses are "
              << kDim0 * kDim1 * kRuns * kIterations << std::endl;

    // Equidistant axis in z - bound
    EquidistantAxisBound zAxis(-400, 400, kDim0);
    // Circular axis in phi
    EquidistantAxisClosed phiAxis(-M_PI, M_PI, kDim1);

    Grid<std::vector<unsigned int>, decltype(zAxis), decltype(phiAxis)>
        zPhiGrid({zAxis, phiAxis});

    // Filling the grid
    for (unsigned int g = 1; g <= kDim0 * kDim1; ++g) {
      zPhiGrid.at(g) = {g, 1000};
    }

    // Create an index grid implementation, with small vector
    GridEntryImpl<decltype(zPhiGrid), small_vector> vToSV2D(
        zPhiGrid, {binZ, binPhi}, Transform3::Identity());

    SymmetricNeighbors<1u, VectorTypeInserter<true>> v;

    const auto vToSV2D_result_n = Acts::Test::microBenchmark(
        [&] {
          run_bench_2D<decltype(vToSV2D), decltype(v)>(vToSV2D, kDim0, kDim1);
        },
        kIterations, kRuns);
    std::cout << " with explicit neighbor search  : " << vToSV2D_result_n
              << std::endl;

    vToSV2D.connectAdjacent<decltype(v)>();

    const auto vToSV2D_result_a = Acts::Test::microBenchmark(
        [&] {
          run_bench_2D<decltype(vToSV2D), BinOnly>(vToSV2D, kDim0, kDim1);
        },
        kIterations, kRuns);
    std::cout << " with adjacent neighbor search  : " << vToSV2D_result_a
              << std::endl;

    const auto vToSV2D_result_d = Acts::Test::microBenchmark(
        [&] { run_bench_2D_bin_access(vToSV2D, kDim0, kDim1); }, kIterations,
        kRuns);
    std::cout << " with direct bin access         : " << vToSV2D_result_d
              << std::endl;
  }
  return 0;
}
