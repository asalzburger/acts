// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Surfaces/CylinderBounds.hpp"
#include "Acts/Surfaces/RadialBounds.hpp"
#include "Acts/Surfaces/RectangleBounds.hpp"
#include "Acts/Surfaces/TrapezoidBounds.hpp"
#include "Acts/Utilities/DirectedProtoAxisHelper.hpp"

#include <numbers>
#include <vector>

using namespace Acts;

namespace ActsTests {

BOOST_AUTO_TEST_SUITE(UtilitiesSuite)

BOOST_AUTO_TEST_CASE(DirectedProtoAxisAdjustment_Radial) {
  using enum AxisDirection;
  using enum AxisBoundaryType;

  RadialBounds bound(50., 75., std::numbers::pi, 0.);
  std::vector<DirectedProtoAxis> dProtoAxes = {
      DirectedProtoAxis(AxisR, Open, 0., 1., 1u),
      DirectedProtoAxis(AxisPhi, Closed, 0., 1., 1u)};

  auto adjusted = adjustDirectedProtoAxes(dProtoAxes, bound);

  BOOST_CHECK_EQUAL(adjusted.at(0).getAxisDirection(), AxisR);
  BOOST_CHECK_EQUAL(adjusted.at(0).getAxis().getMin(), 50.);
  BOOST_CHECK_EQUAL(adjusted.at(0).getAxis().getMax(), 75.);
  BOOST_CHECK_EQUAL(adjusted.at(1).getAxisDirection(), AxisPhi);
  BOOST_CHECK_EQUAL(adjusted.at(1).getAxis().getMin(), -std::numbers::pi);
  BOOST_CHECK_EQUAL(adjusted.at(1).getAxis().getMax(), std::numbers::pi);
}

BOOST_AUTO_TEST_CASE(DirectedProtoAxisAdjustment_Cylinder) {
  using enum AxisDirection;
  using enum AxisBoundaryType;

  CylinderBounds bound(25., 50., std::numbers::pi / 4., 0.);
  std::vector<DirectedProtoAxis> dProtoAxes = {
      DirectedProtoAxis(AxisPhi, Open, 0., 1., 1u),
      DirectedProtoAxis(AxisZ, Open, 0., 1., 1u)};

  auto adjusted = adjustDirectedProtoAxes(dProtoAxes, bound);

  BOOST_CHECK_EQUAL(adjusted.at(0).getAxisDirection(), AxisPhi);
  BOOST_CHECK_EQUAL(adjusted.at(0).getAxis().getMin(), -std::numbers::pi / 4.);
  BOOST_CHECK_EQUAL(adjusted.at(0).getAxis().getMax(), std::numbers::pi / 4.);
  BOOST_CHECK_EQUAL(adjusted.at(1).getAxisDirection(), AxisZ);
  BOOST_CHECK_EQUAL(adjusted.at(1).getAxis().getMin(), -50.);
  BOOST_CHECK_EQUAL(adjusted.at(1).getAxis().getMax(), 50.);
}

BOOST_AUTO_TEST_CASE(DirectedProtoAxisAdjustment_Rectangle) {
  using enum AxisDirection;
  using enum AxisBoundaryType;

  RectangleBounds bound(20., 30.);
  std::vector<DirectedProtoAxis> dProtoAxes = {
      DirectedProtoAxis(AxisX, Open, 0., 1., 1u),
      DirectedProtoAxis(AxisY, Open, 0., 1., 1u)};

  auto adjusted = adjustDirectedProtoAxes(dProtoAxes, bound);

  BOOST_CHECK_EQUAL(adjusted.at(0).getAxisDirection(), AxisX);
  BOOST_CHECK_EQUAL(adjusted.at(0).getAxis().getMin(), -20.);
  BOOST_CHECK_EQUAL(adjusted.at(0).getAxis().getMax(), 20.);
  BOOST_CHECK_EQUAL(adjusted.at(1).getAxisDirection(), AxisY);
  BOOST_CHECK_EQUAL(adjusted.at(1).getAxis().getMin(), -30.);
  BOOST_CHECK_EQUAL(adjusted.at(1).getAxis().getMax(), 30.);
}

BOOST_AUTO_TEST_CASE(DirectedProtoAxisAdjustment_Trapezoid) {
  using enum AxisDirection;
  using enum AxisBoundaryType;

  TrapezoidBounds bound(5., 15., 30.);
  std::vector<DirectedProtoAxis> dProtoAxes = {
      DirectedProtoAxis(AxisX, Open, 0., 1., 1u),
      DirectedProtoAxis(AxisY, Open, 0., 1., 1u)};

  auto adjusted = adjustDirectedProtoAxes(dProtoAxes, bound);

  BOOST_CHECK_EQUAL(adjusted.at(0).getAxisDirection(), AxisX);
  BOOST_CHECK_EQUAL(adjusted.at(0).getAxis().getMin(), -15.);
  BOOST_CHECK_EQUAL(adjusted.at(0).getAxis().getMax(), 15.);
  BOOST_CHECK_EQUAL(adjusted.at(1).getAxisDirection(), AxisY);
  BOOST_CHECK_EQUAL(adjusted.at(1).getAxis().getMin(), -30.);
  BOOST_CHECK_EQUAL(adjusted.at(1).getAxis().getMax(), 30.);
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace ActsTests
