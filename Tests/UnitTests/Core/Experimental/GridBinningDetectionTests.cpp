

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

#include "Acts/Experimental/GridBinningDetection.hpp"

namespace Acts {

/// These tests cover the auto-detection of grid binning, they are
/// performed in an bound and closed case for binning axis
///
namespace Test {

BOOST_AUTO_TEST_SUITE(Experimental)

BOOST_AUTO_TEST_CASE(Equidistant) {
  // Target binning is : equidistant, with approximiate bins:
  //  -5  -4  -3  -2
  //   |   |   |   |
  //
  // This tests the clustering in case of fuzziness, the registerd
  // values come, e.g. from Geometry parsing
  //
  std::vector<ActsScalar> registered = {-5.021, -4.021, -5.011, -4.011, -5.,
                                        -4.,    -4.99,  -3.99,  -3.,    -2.,
                                        -3.005, -2.,    -2.99,  -2.05};

  GridBinningDetection gdb{0.5, 0.05, 0.01, false};

  auto binning = gdb(registered);
  // Check that we have 3 bins
  BOOST_CHECK(std::get<size_t>(binning) == 3u);
  // Check that it is indeed equidistant
  BOOST_CHECK((std::get<std::vector<ActsScalar>>(binning)).empty());
  // Check that the max/min values are set appropriately
  BOOST_CHECK(registered[0] == -5.021);
  BOOST_CHECK(registered[registered.size() - 1u] == -2.);
}

BOOST_AUTO_TEST_CASE(EquidistantMultiplier) {
  // This test checks the possible multiplier detection
  //
  // Input is:
  // [ 0 , 1 , 1.5 , 2 , 3 ]
  //
  // Target binning is:
  //  0   0.5   1   1.5   2.  2.5   3  - equidistant, exact
  //  |    |    |    |    |    |    |
  std::vector<ActsScalar> registered = {0., 1., 1.5, 2., 3.};

  GridBinningDetection gdb{0.01, 0.01, 0.01, false};

  auto binning = gdb(registered);
  // Check that we have 3 bins
  BOOST_CHECK(std::get<size_t>(binning) == 6u);
  // Check that it is indeed equidistant
  BOOST_CHECK((std::get<std::vector<ActsScalar>>(binning)).empty());
  // Check that the max/min values are set appropriately
  BOOST_CHECK(registered[0] == 0.);
  BOOST_CHECK(registered[registered.size() - 1u] == 3.);

  // Redo this test, this time with fuzziness
  registered = {0.002, -0.001, 0., 0.002, 1.01,  1.002, 0.9995, 1.4954,
                1.5,   1.502,  2., 2.002, 1.997, 3.,    2.99};

  binning = gdb(registered);
  // Check that we have 3 bins
  BOOST_CHECK(std::get<size_t>(binning) == 6u);
  // Check that it is indeed equidistant
  BOOST_CHECK((std::get<std::vector<ActsScalar>>(binning)).empty());
  // Check that the max/min values are set appropriately
  BOOST_CHECK(registered[0] == -0.001);
  BOOST_CHECK(registered[registered.size() - 1u] == 3.);
}

BOOST_AUTO_TEST_CASE(Variable) {
  // This test checks the possible multiplier detection
  //
  // Input is:
  // [ 0, 0.1, 0.1, 0.2, 1., 1.5, 1.5, 2. , 2.2, 3. ]
  //
  // Target same with duplicated removed
  //  0   0.1  0.2   1.  1.5  2.  2.2   3  - equidistant, exact
  //  |    |    |    |    |   |    |    |
  std::vector<ActsScalar> registered = {0.,  0.1, 0.1, 0.2, 1.,
                                        1.5, 1.5, 2.,  2.2, 3.};

  GridBinningDetection gdb{0.01, 0.01, 0.01, false};

  auto binning = gdb(registered);
  // Check that we have 3 bins
  BOOST_CHECK(std::get<size_t>(binning) == 7u);
  // Check that it is indeed equidistant
  BOOST_CHECK(not(std::get<std::vector<ActsScalar>>(binning)).empty());
  // Check that the max/min values are set appropriately
  BOOST_CHECK(registered[0] == 0.);
  BOOST_CHECK(registered[registered.size() - 1u] == 3.);
}

BOOST_AUTO_TEST_CASE(EquidistantClosed) {
  // Test for exact binning in phi with -M_PI, M_PI boundary
  // with 6 bins of 1/3 * pi bin size
  //
  // Note this is identical to bound equidistant
  std::vector<ActsScalar> registered = {-M_PI, -2. / 3. * M_PI, -1. / 3. * M_PI,
                                        0.,    1. / 3. * M_PI,  2. / 3. * M_PI,
                                        M_PI};

  GridBinningDetection gdb{0.05, 0.05, 0.05, true};

  auto binning = gdb(registered);

  // Check that we have 6 bins
  BOOST_CHECK(std::get<size_t>(binning) == 6u);
  // Check that it is indeed equidistant
  BOOST_CHECK((std::get<std::vector<ActsScalar>>(binning)).empty());

  // Test with a small rotation, the issue here is that these values
  // are usually gathered by vector::phi parsing, hence all values
  // will have the same shift, except the first/last one, which might
  // fall off either side
  ActsScalar epsilon = 0.01;
  registered = {
      -2. / 3. * M_PI - epsilon, -1. / 3. * M_PI - epsilon, 0. - epsilon,
      1. / 3. * M_PI - epsilon,  2. / 3. * M_PI - epsilon,  M_PI - epsilon};

  // Run the binning test
  binning = gdb(registered);

  // Check that we have 6 bins
  BOOST_CHECK(std::get<size_t>(binning) == 6u);
  // Check that it is indeed equidistant
  BOOST_CHECK((std::get<std::vector<ActsScalar>>(binning)).empty());

  // Check with fussiness - this test should show that
  /// alterations around +/- M_PI are caught
  registered = {-M_PI + epsilon,
                -2. / 3. * M_PI + epsilon,
                -2. / 3. * M_PI - epsilon,
                -1. / 3. * M_PI + epsilon,
                -1. / 3. * M_PI - epsilon,
                0. + epsilon,
                0. - epsilon,
                1. / 3. * M_PI + epsilon,
                1. / 3. * M_PI - epsilon,
                2. / 3. * M_PI + epsilon,
                2. / 3. * M_PI - epsilon,
                M_PI - epsilon};

  // Run the binning test
  binning = gdb(registered);

  // Check that we have 6 bins again, a low one has been inserted
  BOOST_CHECK(std::get<size_t>(binning) == 6u);
  // Check that it is indeed equidistant
  BOOST_CHECK((std::get<std::vector<ActsScalar>>(binning)).empty());

  // Check with symmetric fall-off
  registered = {-0.875 * M_PI, -0.625 * M_PI, -0.375 * M_PI, -0.125 * M_PI,
                0.125 * M_PI,  0.375 * M_PI,  0.625 * M_PI,  0.875 * M_PI};

  // Run the binning test
  binning = gdb(registered);

  // Check that we have 6 bins again, a low one has been inserted
  BOOST_CHECK(std::get<size_t>(binning) == 14u);
  // Check that it is indeed equidistant
  BOOST_CHECK((std::get<std::vector<ActsScalar>>(binning)).empty());
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace Test
}  // namespace Acts