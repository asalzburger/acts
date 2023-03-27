// This file is part of the Acts project.
//
// Copyright (C) 2022 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "Acts/Detector/DetectorHelper.hpp"

#include "Acts/Detector/CylindricalDetectorHelper.hpp"
#include "Acts/Detector/DetectorVolume.hpp"

#include <exception>

namespace {

/// Strip out the detector type from a given set of volumes
///
/// @param volumes the volumes to be connected
///
/// @return a VolumeBoundsType
Acts::VolumeBounds::BoundsType detectorType(
    const std::vector<std::shared_ptr<Acts::Experimental::DetectorVolume>>&
        volumes) {
  // Grab the shape of the reference volume
  const auto& refVolume = volumes.front();
  return refVolume->volumeBounds().type();
}

/// Strip out the detector type from container
///
/// @param containers the containers to be connected
///
/// @return a VolumeBoundsType
Acts::VolumeBounds::BoundsType detectorType(
    const std::vector<Acts::Experimental::ProtoContainer>& containers) {
  // The reference contaienr
  const auto& refContainer = (containers.front());
  if (refContainer.find(2u) != refContainer.end() and
      refContainer.find(2u)->second->surface().type() ==
          Acts::Surface::SurfaceType::Cylinder) {
    return Acts::VolumeBounds::BoundsType::eCylinder;
  }
  return Acts::VolumeBounds::BoundsType::eOther;
}

}  // namespace

Acts::Experimental::ProtoContainer Acts::Experimental::connectDetectorVolumes(
    const GeometryContext& gctx, const std::vector<BinningValue>& bValues,
    std::vector<std::shared_ptr<Experimental::DetectorVolume>>& volumes,
    Acts::Logging::Level logLevel) {
  // Screen output
  ACTS_LOCAL_LOGGER(getDefaultLogger("DetectorHelper", logLevel));

  // Simple dispatcher to make client code a bit more readable
  auto dType = detectorType(volumes);
  // Deal with a cylindrical detector
  if (dType == Acts::VolumeBounds::BoundsType::eCylinder) {
    ACTS_DEBUG("Cylindrical detector detected.");
    if (bValues.size() == 1u) {
      auto bValue = bValues.front();
      switch (bValue) {
        case binR: {
          return CylindricalDetector::connectDetectorVolumesInR(gctx, volumes,
                                                                {}, logLevel);
        };
        case binPhi: {
          return CylindricalDetector::connectDetectorVolumesInPhi(gctx, volumes,
                                                                  {}, logLevel);
        };
        case binZ: {
          return CylindricalDetector::connectDetectorVolumesInZ(gctx, volumes,
                                                                {}, logLevel);
        };
        default:
          break;
      }
    } else if (bValues.size() == 2u) {
      if (bValues[0u] == binZ and bValues[1u] == binR) {
        return CylindricalDetector::wrapDetectorVolumesInZR(gctx, volumes,
                                                            logLevel);
      }
    }
    // This was not successful, throw an exception
    throw std::invalid_argument(
        "DetectorHelper: connector mode not implemented.");
  }
  return ProtoContainer{};
}

Acts::Experimental::ProtoContainer Acts::Experimental::connectContainers(
    const GeometryContext& gctx, const std::vector<BinningValue>& bValues,
    const std::vector<ProtoContainer>& containers,
    Acts::Logging::Level logLevel) {
  // Screen output
  ACTS_LOCAL_LOGGER(getDefaultLogger("DetectorHelper", logLevel));

  // Simple dispatcher to make client code a bit more readable
  auto dType = detectorType(containers);
  // Deal with a cylindrical detector
  if (dType == Acts::VolumeBounds::BoundsType::eCylinder) {
    ACTS_DEBUG("Cylindrical detector detected.");

    // One-dimensional binning
    if (bValues.size() == 1u) {
      auto bValue = bValues.front();
      switch (bValue) {
        case binR: {
          return CylindricalDetector::connectContainersInR(gctx, containers, {},
                                                           logLevel);
        };
        case binZ: {
          return CylindricalDetector::connectContainersInZ(gctx, containers, {},
                                                           logLevel);
        };
        default:
          break;
      }
    }
  }

  throw std::invalid_argument(
      "DetectorHelper: connector mode not implemented.");

  return ProtoContainer{};
}
