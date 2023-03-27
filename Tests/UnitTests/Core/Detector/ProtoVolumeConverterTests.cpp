// This file is part of the Acts project.
//
// Copyright (C) 2022 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Detector/ProtoVolumeConverter.hpp"

#include <exception>
#include <memory>

Acts::GeometryContext tContext;

BOOST_AUTO_TEST_SUITE(Experimental)

BOOST_AUTO_TEST_CASE(CylindricalProtoVolumeConversion) {
  // Full cylinder - as a test of the converter
  Acts::ProtoVolume fullCylinder;
  fullCylinder.name = "full-cylinder";
  fullCylinder.extent.set(Acts::binR, 0., 30.);
  fullCylinder.extent.set(Acts::binZ, -100., 100.);

  Acts::Experimental::ConcentricCylinderConverter fcConverter{fullCylinder};

  auto [fcTransform, fcBounds] = fcConverter.create(tContext);

  BOOST_CHECK(Acts::Transform3::Identity().isApprox(fcTransform));
  BOOST_CHECK(fcBounds->type() == Acts::VolumeBounds::BoundsType::eCylinder);

  // Test as a block builder
  Acts::Experimental::SingleBlockBuilder<> fcBlockBuilder{fullCylinder};

  Acts::Experimental::DetectorBlock dBlock;
  fcBlockBuilder(dBlock, tContext);

  auto& dVolumes = std::get<Acts::Experimental::DetectorVolumes>(dBlock);
  auto& dContainer = std::get<Acts::Experimental::ProtoContainer>(dBlock);

  BOOST_CHECK(dVolumes.size() == 1u);
  BOOST_CHECK(dContainer.size() == 3u);

  // A tube-like cylinder
  Acts::ProtoVolume tubeCylinder;
  tubeCylinder.name = "tube-cylinder";
  tubeCylinder.extent.set(Acts::binR, 10., 30.);
  tubeCylinder.extent.set(Acts::binZ, 100., 200.);

  Acts::Experimental::ConcentricCylinderConverter tcConverter{tubeCylinder};

  auto [tcTransform, tcBounds] = tcConverter.create(tContext);

  Acts::Transform3 shifted = Acts::Transform3::Identity();
  shifted.pretranslate(Acts::Vector3{0., 0., 150.});
  BOOST_CHECK(shifted.isApprox(tcTransform));
  BOOST_CHECK(tcBounds->type() == Acts::VolumeBounds::BoundsType::eCylinder);

  Acts::Experimental::SingleBlockBuilder<> tcBlockBuilder{tubeCylinder};
  dBlock = Acts::Experimental::DetectorBlock{};
  tcBlockBuilder(dBlock, tContext);

  dVolumes = std::get<Acts::Experimental::DetectorVolumes>(dBlock);
  dContainer = std::get<Acts::Experimental::ProtoContainer>(dBlock);

  BOOST_CHECK(dVolumes.size() == 1u);
  BOOST_CHECK(dContainer.size() == 4u);
}

BOOST_AUTO_TEST_SUITE_END()
