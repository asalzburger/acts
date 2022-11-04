// This file is part of the Acts project.
//
// Copyright (C) 2022 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Experimental/CylindricalDetectorHelper.hpp"
#include "Acts/Experimental/Detector.hpp"
#include "Acts/Experimental/DetectorVolume.hpp"
#include "Acts/Experimental/detail/NavigationStateUpdators.hpp"
#include "Acts/Experimental/detail/PortalGenerators.hpp"
#include "Acts/Geometry/CylinderVolumeBounds.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Plugins/ActSVG/DetectorSvgConverter.hpp"
#include "Acts/Utilities/Enumerate.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <fstream>
#include <memory>
#include <vector>

Acts::GeometryContext tgContext;

auto nominal = Acts::Transform3::Identity();

auto portalGenerator = Acts::Experimental::detail::defaultPortalGenerator();

auto navigationStateUpdator =
    Acts::Experimental::detail::allPortals();

using namespace Acts::Experimental;


BOOST_AUTO_TEST_SUITE(DetectorSvgConverter)

BOOST_AUTO_TEST_CASE(TubeSectorCylindricalDetectorVolume) {
  // The central volume definitions
  Acts::ActsScalar rInner = 10.;
  Acts::ActsScalar rOuter = 100.;
  Acts::ActsScalar zHalfL = 300.;

  // The negative/positive definition
  Acts::ActsScalar zPosEC = 350;
  Acts::ActsScalar zHalfLEC = 50.;

  Acts::Transform3 necTranslation = nominal;
  necTranslation.pretranslate(Acts::Vector3(0., 0., -zPosEC));

  auto necCylinderBounds =
      std::make_unique<Acts::CylinderVolumeBounds>(rInner, rOuter, zHalfLEC);

  auto necCylinderVolume = Acts::Experimental::DetectorVolumeFactory::construct(
      portalGenerator, tgContext, "NecCylinderVolume",  necTranslation,
      std::move(necCylinderBounds), navigationStateUpdator);

  auto centralCylinderBounds =
      std::make_unique<Acts::CylinderVolumeBounds>(rInner, rOuter, zHalfL);

  auto centralCylinderVolume =
      Acts::Experimental::DetectorVolumeFactory::construct(
          portalGenerator,  tgContext, "CentralCylinderVolume", nominal,
          std::move(centralCylinderBounds), navigationStateUpdator);

  Acts::Transform3 pecTranslation = nominal;
  pecTranslation.pretranslate(Acts::Vector3(0., 0., zPosEC));

  auto pecCylinderBounds =
      std::make_unique<Acts::CylinderVolumeBounds>(rInner, rOuter, zHalfLEC);

  auto pecCylinderVolume = Acts::Experimental::DetectorVolumeFactory::construct(
      portalGenerator, tgContext, "PecCylinderVolume",  pecTranslation,
      std::move(pecCylinderBounds), navigationStateUpdator);

  // By hand attachment for this test
  if (false) {
    auto& necPortalP = necCylinderVolume->portalPtrs()[1u];
    necPortalP->fuse(*centralCylinderVolume->portalPtrs()[0u].get());
    centralCylinderVolume->updatePortal(necPortalP, 0u);

    auto& centralCylinderP = centralCylinderVolume->portalPtrs()[1u];
    centralCylinderP->fuse(*pecCylinderVolume->portalPtrs()[0u].get());
    pecCylinderVolume->updatePortal(centralCylinderP, 0u);
  } else {
    Acts::Experimental::CylindricalDetectorHelperOptions cOptions;
    cOptions.logLevel = Acts::Logging::VERBOSE;

    Acts::Experimental::connectCylindricalVolumes(
        tgContext,
        {necCylinderVolume, centralCylinderVolume, pecCylinderVolume},
        cOptions);
  }


  std::vector<std::shared_ptr<DetectorVolume>> detectorVolumes = {
      necCylinderVolume, centralCylinderVolume, pecCylinderVolume};
  auto detector =
      Detector::makeShared("Detector", detectorVolumes, detail::tryAllVolumes());

  Acts::Svg::DetectorConverter::Options detectorOptions;

  auto pDetector = Acts::Svg::DetectorConverter::convert(tgContext, *detector,
                                                         detectorOptions);
  pDetector._name = detector->name();

  // Colorize in blue
  actsvg::style::color red({{255, 0, 0}});
  actsvg::style::color green({{0, 255, 0}});
  actsvg::style::color blue({{0, 0, 255}});
  std::vector<actsvg::style::color> colors = {red, green, blue};
  for (auto& c : colors) {
    c._opacity = 0.1;
  }

  pDetector.colorize(colors);

  // As sheet
  auto dv_zr = Acts::Svg::View::zr(pDetector, pDetector._name);
  Acts::Svg::toFile({dv_zr}, pDetector._name + "_zr.svg");
}

BOOST_AUTO_TEST_SUITE_END()