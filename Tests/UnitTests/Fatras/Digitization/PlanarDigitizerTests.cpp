// This file is part of the Acts project.
//
// Copyright (C) 2020 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include <limits>

#include "Acts/Surfaces/PlaneSurface.hpp"
#include "Acts/Surfaces/RectangleBounds.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/Utilities/BinUtility.hpp"
#include "Acts/Utilities/Definitions.hpp"
#include "ActsFatras/Digitization/PlanarDigitizer.hpp"

using namespace ActsFatras;

BOOST_AUTO_TEST_SUITE(FatrasDigitization)

BOOST_AUTO_TEST_CASE(PlanarDigitizerBoundsMasking) {}

BOOST_AUTO_TEST_CASE(PlanarDigitizerCellsCartesian) {
  PlanarDigitizer::Config pdConfig;
  PlanarDigitizer pDigitizer(pdConfig);

  /// Cartesian grid stepper
  ///
  /// @param dLine The digitisation direction
  /// @param ib Towards the next ib boundary
  /// @param bValue The value of the next ib boundary
  ///
  /// @return distance to next ib boundary
  auto pixelStepper = [](const Eigen::ParametrizedLine<double, 2>& dLine,
                         unsigned int ib, float bValue) -> double {
    using Plane2D = Eigen::Hyperplane<double, 2>;
    Acts::Vector2D n(0., 0.);
    n[ib] = 1.;
    return dLine.intersection(Plane2D(n, bValue * n));
  };

  auto rBounds = std::make_shared<const Acts::RectangleBounds>(8., 20.);
  auto rTransform =
      std::make_shared<const Acts::Transform3D>(Acts::Transform3D::Identity());
  auto rPlane =
      Acts::Surface::makeShared<Acts::PlaneSurface>(rTransform, rBounds);

  Acts::Vector2D start2D(-2.38, 4.88);
  Acts::Vector2D end2D(-2.82, 6.23);

  // 0.050 x 0.100 Pixels
  Acts::BinUtility pSegmentation(320, -8., 8., Acts::open, Acts::binX);
  pSegmentation += Acts::BinUtility(400, -20., 20., Acts::open, Acts::binY);

  // 0.2 strips
  Acts::BinUtility sSegmentation(80, -8., 8., Acts::open, Acts::binX);

  // The total length of the projected path
  double pPath = (end2D - start2D).norm();

  auto cells =
      pDigitizer.cellsLocal(pSegmentation, start2D, end2D, pixelStepper);

  unsigned int sb0 = pSegmentation.bin(start2D, 0);
  unsigned int sb1 = pSegmentation.bin(start2D, 1);
  unsigned int eb0 = pSegmentation.bin(end2D, 0);
  unsigned int eb1 = pSegmentation.bin(end2D, 1);

  std::cout << "Digitize [" << sb0 << ", " << sb1 << "] -> [" << eb0 << ", "
            << eb1 << "] @ " << pPath << std::endl;

  unsigned int minb0 = std::numeric_limits<unsigned int>::max();
  unsigned int maxb0 = 0;
  unsigned int minb1 = std::numeric_limits<unsigned int>::max();
  unsigned int maxb1 = 0;

  // Check the cells
  double aPath = 0.;
  for (auto c : cells) {
    aPath += c.data;
    std::cout << "Cell : " << c.channel0 << ", " << c.channel1 << " - with "
              << c.data << " @ " << aPath << std::endl;
    minb0 = std::min(minb0, c.channel0);
    maxb0 = std::max(maxb0, c.channel0);
    minb1 = std::min(minb1, c.channel1);
    maxb1 = std::max(maxb1, c.channel1);
  }

  CHECK_CLOSE_ABS(pPath, aPath, Acts::s_onSurfaceTolerance);
  BOOST_CHECK_EQUAL(std::min(sb0, eb0), minb0);
  BOOST_CHECK_EQUAL(std::max(sb0, eb0), maxb0);
  BOOST_CHECK_EQUAL(std::min(sb1, eb1), minb1);
  BOOST_CHECK_EQUAL(std::max(sb1, eb1), maxb1);
}

BOOST_AUTO_TEST_SUITE_END()
