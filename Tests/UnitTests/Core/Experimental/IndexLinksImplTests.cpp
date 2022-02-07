// This file is part of the Acts project.
//
// Copyright (C) 2022 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <boost/test/data/test_case.hpp>
#include <boost/test/tools/output_test_stream.hpp>
#include <boost/test/unit_test.hpp>

#include "Acts/Experimental/IndexLinksImpl.hpp"
#include "Acts/Utilities/Delegate.hpp"
#include "Acts/Utilities/detail/Axis.hpp"
#include "Acts/Utilities/detail/Grid.hpp"

#include <array>
#include <set>
#include <vector>

namespace Acts {

using namespace detail;

using EquidistantAxisClosed =
    Axis<AxisType::Equidistant, AxisBoundaryType::Closed>;
using EquidistantAxisBound =
    Axis<AxisType::Equidistant, AxisBoundaryType::Bound>;

namespace Test {

BOOST_AUTO_TEST_SUITE(Experimental)

/// Unit tests for Grid entry provider
BOOST_AUTO_TEST_CASE(IndexLinksImpl_EntryConverter_IntegerToInteger) {
  // Test integer entry
  BOOST_CHECK(convert_entry<int>(2) == 2);

  // Test unsigned integer entry
  BOOST_CHECK(convert_entry<unsigned int>(4u) == 4u);
}

/// Unit tests for Grid entry provider
BOOST_AUTO_TEST_CASE(IndexLinksImpl_EntryConverter_ContainerToContainer) {
  // Test array entry
  std::array<int, 2> testa = {7, 2};
  BOOST_CHECK(convert_entry<decltype(testa)>({7, 2}) == testa);

  // Test set entry
  std::set<int> tests = {12, 7, -3};
  BOOST_CHECK(convert_entry<decltype(tests)>({-3, 7, 12}) == tests);

  // Test vector entry
  std::vector<int> testv = {12, 7, -3};
  BOOST_CHECK(convert_entry<decltype(testv)>({12, 7, -3}) == testv);
}

/// Unit tests for Grid entry provider - entry to container
BOOST_AUTO_TEST_CASE(IndexLinksImpl_EntryConverter_EntryToContainer) {
  std::array<int, 1> testa = {1};
  auto converteda = convert_entry<int, decltype(testa)>(1);
  BOOST_CHECK(converteda == testa);

  std::set<int> tests = {2};
  auto converteds = convert_entry<int, decltype(tests)>(2);
  BOOST_CHECK(converteds == tests);

  std::vector<int> testv = {3};
  auto convertedv = convert_entry<int, decltype(testv)>(3);
  BOOST_CHECK(convertedv == testv);
}

/// Unit tests for Grid entry provider - container a to container b
BOOST_AUTO_TEST_CASE(IndexLinksImpl_EntryConverter_ContainerAToContainerB) {
  // Test array entry return as vector
  std::array<int, 1> entrya = {1};
  std::vector<int> resultv = {1};

  auto converteda = convert_entry<decltype(entrya), decltype(resultv)>(entrya);
  BOOST_CHECK(converteda == resultv);

  // Test set entry return as vector
  std::set<int> entrys = {1};

  auto converteds = convert_entry<decltype(entrys), decltype(resultv)>(entrys);
  BOOST_CHECK(converteds == resultv);

  // Test vector entry return as set
  std::vector<int> entryv = {1};
  std::set<int> results = {1};

  auto convertedv = convert_entry<decltype(entryv), decltype(results)>(entryv);
  BOOST_CHECK(convertedv == results);
}

/// Test link implementation of single entry
BOOST_AUTO_TEST_CASE(IndexLinksImpl_SingleEntry) {
  // Equidistant axis
  EquidistantAxisBound eAxis(0., 10., 10u);
  Grid<unsigned int, decltype(eAxis)> indexGrid(eAxis);

  // Grid structure:
  //
  // g :  0 |  1  |  2  |  3  |  4  |  5  |  6  |  7  |  8  |  9  |
  //    ----|-----|-----|-----|-----|-----|-----|-----|-----|-----|
  //    101 | 102 | 103 | 104 | 105 | 106 | 107 | 108 | 109 | 110 |
  //
  indexGrid.at(1) = 101;
  indexGrid.at(2) = 102;
  indexGrid.at(3) = 103;
  indexGrid.at(4) = 104;
  indexGrid.at(5) = 105;
  indexGrid.at(6) = 106;
  indexGrid.at(7) = 107;
  indexGrid.at(8) = 108;
  indexGrid.at(9) = 109;
  indexGrid.at(10) = 110;

  GridEntryImpl<decltype(indexGrid)> gridEntryImpl(std::move(indexGrid), {binX},
                                                   Transform3::Identity());

  BOOST_CHECK(gridEntryImpl.links<BinOnly>(Vector3{0.5, 0., 0.}) == 101);
  BOOST_CHECK(gridEntryImpl.links<BinOnly>(Vector3{1.5, 0., 0.}) == 102);
  BOOST_CHECK(gridEntryImpl.links<BinOnly>(Vector3{2.5, 0., 0.}) == 103);
  BOOST_CHECK(gridEntryImpl.links<BinOnly>(Vector3{3.5, 0., 0.}) == 104);
  BOOST_CHECK(gridEntryImpl.links<BinOnly>(Vector3{4.5, 0., 0.}) == 105);
  BOOST_CHECK(gridEntryImpl.links<BinOnly>(Vector3{5.5, 0., 0.}) == 106);
  BOOST_CHECK(gridEntryImpl.links<BinOnly>(Vector3{6.5, 0., 0.}) == 107);
  BOOST_CHECK(gridEntryImpl.links<BinOnly>(Vector3{7.5, 0., 0.}) == 108);
  BOOST_CHECK(gridEntryImpl.links<BinOnly>(Vector3{8.5, 0., 0.}) == 109);
  BOOST_CHECK(gridEntryImpl.links<BinOnly>(Vector3{9.5, 0., 0.}) == 110);
}

/// Test link implementation singl eto vector
BOOST_AUTO_TEST_CASE(IndexLinksImpl_SingleEntryToVector) {
  // Equidistant axis
  EquidistantAxisBound eAxis(0., 10., 10u);
  Grid<unsigned int, decltype(eAxis)> indexGrid(eAxis);

  // Grid structure:
  //
  // g :  0 |  1  |  2  |  3  |  4  |  5  |  6  |  7  |  8  |  9  |
  //    ----|-----|-----|-----|-----|-----|-----|-----|-----|-----|
  //    101 | 102 | 103 | 104 | 105 | 106 | 107 | 108 | 109 | 110 |
  //
  indexGrid.at(1) = 101;
  indexGrid.at(2) = 102;
  indexGrid.at(3) = 103;
  indexGrid.at(4) = 104;
  indexGrid.at(5) = 105;
  indexGrid.at(6) = 106;
  indexGrid.at(7) = 107;
  indexGrid.at(8) = 108;
  indexGrid.at(9) = 109;
  indexGrid.at(10) = 110;

  GridEntryImpl<decltype(indexGrid), std::vector<unsigned int>> gridEntryImpl(
      std::move(indexGrid), {binX}, Transform3::Identity());

  using return_type = decltype(gridEntryImpl)::return_type;

  BOOST_CHECK(gridEntryImpl.links<BinOnly>(Vector3{0.5, 0., 0.}) ==
              return_type{101});
  BOOST_CHECK(gridEntryImpl.links<BinOnly>(Vector3{1.5, 0., 0.}) ==
              return_type{102});
  BOOST_CHECK(gridEntryImpl.links<BinOnly>(Vector3{2.5, 0., 0.}) ==
              return_type{103});
  BOOST_CHECK(gridEntryImpl.links<BinOnly>(Vector3{3.5, 0., 0.}) ==
              return_type{104});
  BOOST_CHECK(gridEntryImpl.links<BinOnly>(Vector3{4.5, 0., 0.}) ==
              return_type{105});
  BOOST_CHECK(gridEntryImpl.links<BinOnly>(Vector3{5.5, 0., 0.}) ==
              return_type{106});
  BOOST_CHECK(gridEntryImpl.links<BinOnly>(Vector3{6.5, 0., 0.}) ==
              return_type{107});
  BOOST_CHECK(gridEntryImpl.links<BinOnly>(Vector3{7.5, 0., 0.}) ==
              return_type{108});
  BOOST_CHECK(gridEntryImpl.links<BinOnly>(Vector3{8.5, 0., 0.}) ==
              return_type{109});
  BOOST_CHECK(gridEntryImpl.links<BinOnly>(Vector3{9.5, 0., 0.}) ==
              return_type{110});
}

/// Test link implementation singl eto vector
BOOST_AUTO_TEST_CASE(IndexLinksImpl_SingleEntryToVector_W_Neibgbor) {
  // Equidistant axis
  EquidistantAxisBound eAxis(0., 10., 10u);
  Grid<unsigned int, decltype(eAxis)> indexGrid(eAxis);

  // Grid structure:
  //
  // g :  0 |  1  |  2  |  3  |  4  |  5  |  6  |  7  |  8  |  9  |
  //    ----|-----|-----|-----|-----|-----|-----|-----|-----|-----|
  //    101 | 102 | 103 | 104 | 105 | 106 | 107 | 108 | 109 | 110 |
  //
  indexGrid.at(1) = 101;
  indexGrid.at(2) = 102;
  indexGrid.at(3) = 103;
  indexGrid.at(4) = 104;
  indexGrid.at(5) = 105;
  indexGrid.at(6) = 106;
  indexGrid.at(7) = 107;
  indexGrid.at(8) = 108;
  indexGrid.at(9) = 109;
  indexGrid.at(10) = 110;

  GridEntryImpl<decltype(indexGrid), std::vector<unsigned int>> gridEntryImpl(
      std::move(indexGrid), {binX}, Transform3::Identity());

  using return_type = decltype(gridEntryImpl)::return_type;

  SymmetricNeighbors<1u> snh1;

  return_type reference = {101, 102};
  BOOST_CHECK(gridEntryImpl.links<decltype(snh1)>(Vector3{0.5, 0., 0.}) ==
              reference);
  reference = {101, 102, 103};
  BOOST_CHECK(gridEntryImpl.links<decltype(snh1)>(Vector3{1.5, 0., 0.}) ==
              reference);
  reference = {102, 103, 104};
  BOOST_CHECK(gridEntryImpl.links<decltype(snh1)>(Vector3{2.5, 0., 0.}) ==
              reference);
  reference = {103, 104, 105};
  BOOST_CHECK(gridEntryImpl.links<decltype(snh1)>(Vector3{3.5, 0., 0.}) ==
              reference);
  reference = {104, 105, 106};
  BOOST_CHECK(gridEntryImpl.links<decltype(snh1)>(Vector3{4.5, 0., 0.}) ==
              reference);
  reference = {105, 106, 107};
  BOOST_CHECK(gridEntryImpl.links<decltype(snh1)>(Vector3{5.5, 0., 0.}) ==
              reference);
  reference = {106, 107, 108};
  BOOST_CHECK(gridEntryImpl.links<decltype(snh1)>(Vector3{6.5, 0., 0.}) ==
              reference);
  reference = {107, 108, 109};
  BOOST_CHECK(gridEntryImpl.links<decltype(snh1)>(Vector3{7.5, 0., 0.}) ==
              reference);
  reference = {108, 109, 110};
  BOOST_CHECK(gridEntryImpl.links<decltype(snh1)>(Vector3{8.5, 0., 0.}) ==
              reference);
  reference = {109, 110};
  BOOST_CHECK(gridEntryImpl.links<decltype(snh1)>(Vector3{9.5, 0., 0.}) ==
              reference);
}

/// Test link implementation singl eto vector
BOOST_AUTO_TEST_CASE(IndexLinksImpl_VectorToVector_W_Neibgbor_WO_Duplicates) {
  // Equidistant axis
  EquidistantAxisBound eAxis(0., 10., 10u);
  Grid<std::vector<unsigned int>, decltype(eAxis)> indexGrid(eAxis);

  // Grid structure:
  //
  // g :   0 |   1  |   2  |   3  |   4  |  5 |  6 |  7 |  8 |  9 |
  //    -----|------|------|------|------|----|----|----|----|----|
  //    11,1 | 12,1 | 13,1 | 14,1 | 15,1 | 16 | 17 | 18 | 19 | 20 |
  //
  indexGrid.at(1) = {11, 1};
  indexGrid.at(2) = {12, 1};
  indexGrid.at(3) = {13, 1};
  indexGrid.at(4) = {14, 1};
  indexGrid.at(5) = {15, 1};
  indexGrid.at(6) = {16};
  indexGrid.at(7) = {17};
  indexGrid.at(8) = {18};
  indexGrid.at(9) = {19};
  indexGrid.at(10) = {20};

  GridEntryImpl<decltype(indexGrid), std::vector<unsigned int>> gridEntryImpl(
      std::move(indexGrid), {binX}, Transform3::Identity());

  using return_type = decltype(gridEntryImpl)::return_type;

  SymmetricNeighbors<1u, VectorTypeInserter<true>> snh1;

  return_type reference = {1, 11, 12};
  BOOST_CHECK(gridEntryImpl.links<decltype(snh1)>(Vector3{0.5, 0., 0.}) ==
              reference);
  reference = {1, 11, 12, 13};
  BOOST_CHECK(gridEntryImpl.links<decltype(snh1)>(Vector3{1.5, 0., 0.}) ==
              reference);
  reference = {1, 12, 13, 14};
  BOOST_CHECK(gridEntryImpl.links<decltype(snh1)>(Vector3{2.5, 0., 0.}) ==
              reference);
  reference = {1, 13, 14, 15};
  BOOST_CHECK(gridEntryImpl.links<decltype(snh1)>(Vector3{3.5, 0., 0.}) ==
              reference);
  reference = {1, 14, 15, 16};
  BOOST_CHECK(gridEntryImpl.links<decltype(snh1)>(Vector3{4.5, 0., 0.}) ==
              reference);
  reference = {1, 15, 16, 17};
  BOOST_CHECK(gridEntryImpl.links<decltype(snh1)>(Vector3{5.5, 0., 0.}) ==
              reference);
  reference = {16, 17, 18};
  BOOST_CHECK(gridEntryImpl.links<decltype(snh1)>(Vector3{6.5, 0., 0.}) ==
              reference);
  reference = {17, 18, 19};
  BOOST_CHECK(gridEntryImpl.links<decltype(snh1)>(Vector3{7.5, 0., 0.}) ==
              reference);
  reference = {18, 19, 20};
  BOOST_CHECK(gridEntryImpl.links<decltype(snh1)>(Vector3{8.5, 0., 0.}) ==
              reference);
  reference = {19, 20};
  BOOST_CHECK(gridEntryImpl.links<decltype(snh1)>(Vector3{9.5, 0., 0.}) ==
              reference);
}

/// Test link implementation array to vector
BOOST_AUTO_TEST_CASE(IndexLinksImpl_ArrayToVector) {
  // Equidistant axis
  EquidistantAxisBound eAxis(0., 10., 10u);
  Grid<std::array<unsigned int, 1>, decltype(eAxis)> indexGrid(eAxis);

  // Grid structure:
  //
  // g :  0 |  1  |  2  |  3  |  4  |  5  |  6  |  7  |  8  |  9  |
  //    ----|-----|-----|-----|-----|-----|-----|-----|-----|-----|
  //    101 | 102 | 103 | 104 | 105 | 106 | 107 | 108 | 109 | 110 |
  //
  indexGrid.at(1) = {101};
  indexGrid.at(2) = {102};
  indexGrid.at(3) = {103};
  indexGrid.at(4) = {104};
  indexGrid.at(5) = {105};
  indexGrid.at(6) = {106};
  indexGrid.at(7) = {107};
  indexGrid.at(8) = {108};
  indexGrid.at(9) = {109};
  indexGrid.at(10) = {110};

  GridEntryImpl<decltype(indexGrid), std::vector<unsigned int>> gridEntryImpl(
      std::move(indexGrid), {binX}, Transform3::Identity());

  using return_type = decltype(gridEntryImpl)::return_type;

  BOOST_CHECK(gridEntryImpl.links<BinOnly>(Vector3{0.5, 0., 0.}) ==
              return_type{101});
  BOOST_CHECK(gridEntryImpl.links<BinOnly>(Vector3{1.5, 0., 0.}) ==
              return_type{102});
  BOOST_CHECK(gridEntryImpl.links<BinOnly>(Vector3{2.5, 0., 0.}) ==
              return_type{103});
  BOOST_CHECK(gridEntryImpl.links<BinOnly>(Vector3{3.5, 0., 0.}) ==
              return_type{104});
  BOOST_CHECK(gridEntryImpl.links<BinOnly>(Vector3{4.5, 0., 0.}) ==
              return_type{105});
  BOOST_CHECK(gridEntryImpl.links<BinOnly>(Vector3{5.5, 0., 0.}) ==
              return_type{106});
  BOOST_CHECK(gridEntryImpl.links<BinOnly>(Vector3{6.5, 0., 0.}) ==
              return_type{107});
  BOOST_CHECK(gridEntryImpl.links<BinOnly>(Vector3{7.5, 0., 0.}) ==
              return_type{108});
  BOOST_CHECK(gridEntryImpl.links<BinOnly>(Vector3{8.5, 0., 0.}) ==
              return_type{109});
  BOOST_CHECK(gridEntryImpl.links<BinOnly>(Vector3{9.5, 0., 0.}) ==
              return_type{110});
}

/// Test link implementation vector to vector
BOOST_AUTO_TEST_CASE(IndexLinksImpl_VectorToVector) {
  // Equidistant axis
  EquidistantAxisBound eAxis(0., 10., 10u);
  Grid<std::vector<unsigned int>, decltype(eAxis)> indexGrid(eAxis);

  // Grid structure:
  //
  // g :  0 |  1  |  2  |  3  |  4  |  5  |  6  |  7  |  8  |  9  |
  //    ----|-----|-----|-----|-----|-----|-----|-----|-----|-----|
  //    101 | 102 | 103 | 104 | 105 | 106 | 107 | 108 | 109 | 110 |
  //
  indexGrid.at(1) = {101};
  indexGrid.at(2) = {102};
  indexGrid.at(3) = {103};
  indexGrid.at(4) = {104};
  indexGrid.at(5) = {105};
  indexGrid.at(6) = {106};
  indexGrid.at(7) = {107};
  indexGrid.at(8) = {108};
  indexGrid.at(9) = {109};
  indexGrid.at(10) = {110};

  GridEntryImpl<decltype(indexGrid), std::vector<unsigned int>> gridEntryImpl(
      std::move(indexGrid), {binX}, Transform3::Identity());

  using return_type = decltype(gridEntryImpl)::return_type;

  BOOST_CHECK(gridEntryImpl.links<BinOnly>(Vector3{0.5, 0., 0.}) ==
              return_type{101});
  BOOST_CHECK(gridEntryImpl.links<BinOnly>(Vector3{1.5, 0., 0.}) ==
              return_type{102});
  BOOST_CHECK(gridEntryImpl.links<BinOnly>(Vector3{2.5, 0., 0.}) ==
              return_type{103});
  BOOST_CHECK(gridEntryImpl.links<BinOnly>(Vector3{3.5, 0., 0.}) ==
              return_type{104});
  BOOST_CHECK(gridEntryImpl.links<BinOnly>(Vector3{4.5, 0., 0.}) ==
              return_type{105});
  BOOST_CHECK(gridEntryImpl.links<BinOnly>(Vector3{5.5, 0., 0.}) ==
              return_type{106});
  BOOST_CHECK(gridEntryImpl.links<BinOnly>(Vector3{6.5, 0., 0.}) ==
              return_type{107});
  BOOST_CHECK(gridEntryImpl.links<BinOnly>(Vector3{7.5, 0., 0.}) ==
              return_type{108});
  BOOST_CHECK(gridEntryImpl.links<BinOnly>(Vector3{8.5, 0., 0.}) ==
              return_type{109});
  BOOST_CHECK(gridEntryImpl.links<BinOnly>(Vector3{9.5, 0., 0.}) ==
              return_type{110});
}

/// Test link implementation vector to set entry
BOOST_AUTO_TEST_CASE(IndexLinksImpl_VectorToSet) {
  // Equidistant axis
  EquidistantAxisBound eAxis(0., 10., 10u);
  Grid<std::vector<unsigned int>, decltype(eAxis)> indexGrid(eAxis);

  // Grid structure:
  //
  // g :  0 |  1  |  2  |  3  |  4  |  5  |  6  |  7  |  8  |  9  |
  //    ----|-----|-----|-----|-----|-----|-----|-----|-----|-----|
  //    101 | 102 | 103 | 104 | 105 | 106 | 107 | 108 | 109 | 110 |
  //
  indexGrid.at(1) = {101};
  indexGrid.at(2) = {102};
  indexGrid.at(3) = {103};
  indexGrid.at(4) = {104};
  indexGrid.at(5) = {105};
  indexGrid.at(6) = {106};
  indexGrid.at(7) = {107};
  indexGrid.at(8) = {108};
  indexGrid.at(9) = {109};
  indexGrid.at(10) = {110};

  GridEntryImpl<decltype(indexGrid), std::set<unsigned int>> gridEntryImpl(
      std::move(indexGrid), {binX}, Transform3::Identity());

  using return_type = decltype(gridEntryImpl)::return_type;

  BOOST_CHECK(gridEntryImpl.links<BinOnly>(Vector3{0.5, 0., 0.}) ==
              return_type{101});
  BOOST_CHECK(gridEntryImpl.links<BinOnly>(Vector3{1.5, 0., 0.}) ==
              return_type{102});
  BOOST_CHECK(gridEntryImpl.links<BinOnly>(Vector3{2.5, 0., 0.}) ==
              return_type{103});
  BOOST_CHECK(gridEntryImpl.links<BinOnly>(Vector3{3.5, 0., 0.}) ==
              return_type{104});
  BOOST_CHECK(gridEntryImpl.links<BinOnly>(Vector3{4.5, 0., 0.}) ==
              return_type{105});
  BOOST_CHECK(gridEntryImpl.links<BinOnly>(Vector3{5.5, 0., 0.}) ==
              return_type{106});
  BOOST_CHECK(gridEntryImpl.links<BinOnly>(Vector3{6.5, 0., 0.}) ==
              return_type{107});
  BOOST_CHECK(gridEntryImpl.links<BinOnly>(Vector3{7.5, 0., 0.}) ==
              return_type{108});
  BOOST_CHECK(gridEntryImpl.links<BinOnly>(Vector3{8.5, 0., 0.}) ==
              return_type{109});
  BOOST_CHECK(gridEntryImpl.links<BinOnly>(Vector3{9.5, 0., 0.}) ==
              return_type{110});
}

/// Test link implementation vector to vector entry - 2D
BOOST_AUTO_TEST_CASE(IndexLinksImpl_VectorToVector_2D_W_and_WO_Neighbors) {
  // Equidistant axis in z - bound
  EquidistantAxisBound zAxis(0., 4., 4u);
  // Circular axis in phi
  EquidistantAxisClosed phiAxis(-M_PI, M_PI, 5u);

  Grid<std::vector<unsigned int>, decltype(zAxis), decltype(phiAxis)> indexGrid(
      {zAxis, phiAxis});

  // This emulates a Cylindrical layer
  //
  // Grid structure:
  //
  //       |  z0   |   z1  |   z2  |   z3  |
  //    ---|-------|-------|-------|-------|
  //    p0 | 101,1 |   102 |   103 |   104 |
  //    p1 |   201 |   202 |   203 |   204 |
  //    p2 |   301 |   302 |   303 |   304 |
  //    p3 |   401 |   402 |   403 |   404 |
  //    p4 | 501,2 | 502,2 | 503,2 | 504,2 |

  indexGrid.atLocalBins({1, 1}) = {101, 1};
  indexGrid.atLocalBins({2, 1}) = {102};
  indexGrid.atLocalBins({3, 1}) = {103};
  indexGrid.atLocalBins({4, 1}) = {104};
  indexGrid.atLocalBins({1, 2}) = {201};
  indexGrid.atLocalBins({2, 2}) = {202};
  indexGrid.atLocalBins({3, 2}) = {203};
  indexGrid.atLocalBins({4, 2}) = {204};
  indexGrid.atLocalBins({1, 3}) = {301};
  indexGrid.atLocalBins({2, 3}) = {302};
  indexGrid.atLocalBins({3, 3}) = {303};
  indexGrid.atLocalBins({4, 3}) = {304};
  indexGrid.atLocalBins({1, 4}) = {401};
  indexGrid.atLocalBins({2, 4}) = {402};
  indexGrid.atLocalBins({3, 4}) = {403};
  indexGrid.atLocalBins({4, 4}) = {404};
  indexGrid.atLocalBins({1, 5}) = {501, 2};
  indexGrid.atLocalBins({2, 5}) = {502, 2};
  indexGrid.atLocalBins({3, 5}) = {503, 2};
  indexGrid.atLocalBins({4, 5}) = {504, 2};

  GridEntryImpl<decltype(indexGrid), std::vector<unsigned int>> gridEntryImpl(
      std::move(indexGrid), {binZ, binPhi}, Transform3::Identity());

  // Check first bin
  using return_type = decltype(gridEntryImpl)::return_type;

  return_type reference = {101, 1};
  BOOST_CHECK(gridEntryImpl.links<BinOnly>(Vector3(-1., -0.05, 0.5)) ==
              reference);
  // Second
  reference = {102};
  BOOST_CHECK(gridEntryImpl.links<BinOnly>(Vector3(-1., -0.05, 1.5)) ==
              reference);
  // ... one in the middle
  reference = {303};
  BOOST_CHECK(gridEntryImpl.links<BinOnly>(Vector3(0., 0.0, 2.5)) == reference);
  // Some in the last row
  reference = {502, 2};
  BOOST_CHECK(gridEntryImpl.links<BinOnly>(Vector3(-1., 0.05, 1.5)) ==
              reference);

  // Check with neigborhood
  SymmetricNeighbors<1u, VectorTypeInserter<true>> snh1;

  // The job is (almost) done here if this works
  reference = {1, 2, 101, 102, 103, 201, 202, 203, 501, 502, 503};
  BOOST_CHECK(gridEntryImpl.links<decltype(snh1)>(Vector3(-1., -0.05, 1.5)) ==
              reference);

  // Call the adjecent bin filling method
  gridEntryImpl.connectAdjacent<decltype(snh1)>();

  // Grid structure - after connectAdjacent<>(..) call:
  //
  //       |  z0         |  z1         | ...
  //    ---|-------------|-------------|----
  //       | 1,2         | 1,2         |
  //       | 504,501,502 | 501,502,503 |
  //    p0 | 104,101,102 | 101,102,103 |
  //       | 204,201,202 | 201,202,203 |
  //    ---|-------------|-------------|
  //       | ...

  // Now it should reproduce the reference w/o calling the nieighborhood
  BOOST_CHECK(gridEntryImpl.links<BinOnly>(Vector3(-1., -0.005, 1.5)) ==
              reference);

  // Test the direct links access, targetting z1, p0
  std::vector<unsigned int> referenceB = {1,   2,   101, 102, 103, 201,
                                          202, 203, 501, 502, 503};
  BOOST_CHECK(gridEntryImpl.links(Vector3(-1., -0.05, 1.5)) == referenceB);

  // We are done here
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace Test
}  // namespace Acts