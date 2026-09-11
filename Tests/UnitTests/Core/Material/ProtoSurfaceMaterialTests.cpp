// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Material/BinnedSurfaceMaterial.hpp"
#include "Acts/Material/ProtoSurfaceMaterial.hpp"
#include "Acts/Utilities/AxisSpec.hpp"
#include "Acts/Utilities/BinUtility.hpp"
#include "Acts/Utilities/BinningType.hpp"
#include "Acts/Utilities/MultiAxisSpec.hpp"

#include <numbers>
#include <utility>
#include <vector>

using namespace Acts;

namespace ActsTests {

BOOST_AUTO_TEST_SUITE(MaterialSuite)

/// Test the constructors
BOOST_AUTO_TEST_CASE(ProtoSurfaceMaterial_construction_test) {
  using enum AxisDirection;

  MultiAxisSpec2D binning({AxisSpec::DeferredEquidistant(10, AxisX),
                           AxisSpec::DeferredEquidistant(10, AxisY)});

  // Constructor from arguments
  ProtoSurfaceMaterial smp(binning);
  BOOST_REQUIRE(smp.binning().has_value());
  BOOST_CHECK_EQUAL(smp.binning()->axisSpec(0).nBins(), 10u);

  // Copy constructor
  ProtoSurfaceMaterial smpCopy(smp);
  BOOST_CHECK(smpCopy.binning() == smp.binning());

  // Copy move constructor
  ProtoSurfaceMaterial smpCopyMoved(std::move(smpCopy));
  BOOST_REQUIRE(smpCopyMoved.binning().has_value());

  // Unbinned proto material marks a surface for homogeneous material
  ProtoSurfaceMaterial homogeneous;
  BOOST_CHECK(!homogeneous.binning().has_value());

  // The axis directions are deliberately not advertised - the spec is
  // resolved against the surface during mapping instead
  BOOST_CHECK(smp.localAxisDirections().empty());
}

/// Test the transitional BinUtility conversions that live with the retired
/// binning classes
BOOST_AUTO_TEST_CASE(ProtoSurfaceMaterial_binUtility_conversion_test) {
  using enum AxisDirection;

  // A dimensionless bin utility has no binning to express
  BOOST_CHECK(!binUtilityToMultiAxisSpec(BinUtility()).has_value());

  // 2D equidistant, ranges are dropped and left to the surface
  BinUtility bu2D(4, -10., 10., open, AxisX);
  bu2D += BinUtility(2, -5., 5., open, AxisY);
  auto spec2D = binUtilityToMultiAxisSpec(bu2D);
  BOOST_REQUIRE(spec2D.has_value());
  BOOST_CHECK_EQUAL(spec2D->axisSpec(0).nBins(), 4u);
  BOOST_CHECK_EQUAL(spec2D->axisSpec(0).direction().value(), AxisX);
  BOOST_CHECK_EQUAL(spec2D->axisSpec(1).nBins(), 2u);
  BOOST_CHECK_EQUAL(spec2D->axisSpec(1).direction().value(), AxisY);
  BOOST_CHECK(spec2D->isDeferred());

  // 1D is padded with a single bin along the partner direction
  auto spec1D = binUtilityToMultiAxisSpec(BinUtility(3, -1., 1., open, AxisR));
  BOOST_REQUIRE(spec1D.has_value());
  BOOST_CHECK_EQUAL(spec1D->axisSpec(0).nBins(), 3u);
  BOOST_CHECK_EQUAL(spec1D->axisSpec(0).direction().value(), AxisR);
  BOOST_CHECK_EQUAL(spec1D->axisSpec(1).nBins(), 1u);
  BOOST_CHECK_EQUAL(spec1D->axisSpec(1).direction().value(), AxisPhi);

  // Azimuthal cylinder binning is normalised onto the canonical rPhi axis
  BinUtility buCyl(8, -std::numbers::pi, std::numbers::pi, closed, AxisPhi);
  buCyl += BinUtility(5, -100., 100., open, AxisZ);
  auto specCyl = binUtilityToMultiAxisSpec(buCyl);
  BOOST_REQUIRE(specCyl.has_value());
  BOOST_CHECK_EQUAL(specCyl->axisSpec(0).direction().value(), AxisRPhi);
  BOOST_CHECK_EQUAL(specCyl->axisSpec(0).nBins(), 8u);
  BOOST_CHECK_EQUAL(specCyl->axisSpec(1).direction().value(), AxisZ);

  // ... while a disc keeps its phi axis
  BinUtility buDisc(6, 30., 80., open, AxisR);
  buDisc += BinUtility(8, -std::numbers::pi, std::numbers::pi, closed, AxisPhi);
  auto specDisc = binUtilityToMultiAxisSpec(buDisc);
  BOOST_REQUIRE(specDisc.has_value());
  BOOST_CHECK_EQUAL(specDisc->axisSpec(1).direction().value(), AxisPhi);

  // Variable binning keeps the relative position of its edges
  std::vector<float> varEdges{0.f, 20.f, 100.f};
  BinUtility buVar(varEdges, open, AxisR);
  auto specVar = binUtilityToMultiAxisSpec(buVar);
  BOOST_REQUIRE(specVar.has_value());
  BOOST_REQUIRE(specVar->axisSpec(0).isDeferredVariable());
  const auto& edges = specVar->axisSpec(0).asDeferredVariable().normalizedEdges;
  BOOST_REQUIRE_EQUAL(edges.size(), 3u);
  BOOST_CHECK_CLOSE(edges[1], 0.2, 1e-6);

  // The bin structure survives the round trip through the legacy format
  BinUtility roundTrip = multiAxisSpecToBinUtility(*spec2D);
  auto specRoundTrip = binUtilityToMultiAxisSpec(roundTrip);
  BOOST_REQUIRE(specRoundTrip.has_value());
  BOOST_CHECK(*specRoundTrip == *spec2D);

  // More than two dimensions cannot be a surface grid
  BinUtility bu3D(2, -1., 1., open, AxisX);
  bu3D += BinUtility(2, -1., 1., open, AxisY);
  bu3D += BinUtility(2, -1., 1., open, AxisZ);
  BOOST_CHECK_THROW(binUtilityToMultiAxisSpec(bu3D), std::invalid_argument);
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace ActsTests
