// This file is part of the Acts project.
//
// Copyright (C) 2019 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Geometry/TrapezoidVolumeBounds.hpp"
#include "Acts/Surfaces/PlanarBounds.hpp"
#include "Acts/Surfaces/PlaneSurface.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/Utilities/BoundingBox.hpp"
#include "Acts/Utilities/Definitions.hpp"

namespace tt = boost::test_tools;

namespace Acts {

namespace Test {
BOOST_AUTO_TEST_SUITE(Volumes)

BOOST_AUTO_TEST_CASE(bounding_box_creation) {
  float tol = 1e-4;

  TrapezoidVolumeBounds tvb(5, 10, 8, 4);

  auto bb = tvb.boundingBox();
  CHECK_CLOSE_ABS(bb.max(), Vector3D(10, 8, 4), tol);
  CHECK_CLOSE_ABS(bb.min(), Vector3D(-10, -8, -4), tol);

  Transform3D trf;

  trf = Translation3D(Vector3D(0, 30, 20));

  bb = tvb.boundingBox(&trf);
  CHECK_CLOSE_ABS(bb.max(), Vector3D(10, 38, 24), tol);
  CHECK_CLOSE_ABS(bb.min(), Vector3D(-10, 22, 16), tol);

  trf = AngleAxis3D(M_PI / 2., Vector3D(-2, 4, 5).normalized());

  bb = tvb.boundingBox(&trf);
  CHECK_CLOSE_ABS(bb.max(), Vector3D(9.32577, 11.4906, 11.5777), tol);
  CHECK_CLOSE_ABS(bb.min(), Vector3D(-9.77021, -8.65268, -9.23688), tol);
}

BOOST_AUTO_TEST_CASE(TrapezoidVolumeBoundarySurfaces) {
  TrapezoidVolumeBounds tvb(5, 10, 8, 4);

  auto tvbOrientedSurfaces = tvb.orientedSurfaces(nullptr);
  BOOST_TEST(tvbOrientedSurfaces.size(), 6);

  for (auto& os : tvbOrientedSurfaces) {
    auto geoCtx = GeometryContext();
    auto osCenter = os.first->center(geoCtx);
    auto osNormal = os.first->normal(geoCtx, osCenter);
    double nDir = (double)os.second;
    // Check if you step inside the volume with the oriented normal
    auto insideTvb = osCenter + nDir * osNormal;
    auto outsideTvb = osCenter - nDir * osNormal;
    BOOST_CHECK(tvb.inside(insideTvb));
    BOOST_CHECK(!tvb.inside(outsideTvb));
  }
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace Test

}  // namespace Acts
