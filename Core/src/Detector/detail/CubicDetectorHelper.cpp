// This file is part of the Acts project.
//
// Copyright (C) 2023 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "Acts/Detector/detail/CubicDetectorHelper.hpp"

#include "Acts/Definitions/Common.hpp"
#include "Acts/Detector/DetectorVolume.hpp"
#include "Acts/Detector/Portal.hpp"
#include "Acts/Detector/detail/ConsistencyChecker.hpp"
#include "Acts/Detector/detail/PortalHelper.hpp"
#include "Acts/Geometry/CuboidVolumeBounds.hpp"
#include "Acts/Surfaces/PlaneSurface.hpp"
#include "Acts/Surfaces/RectangleBounds.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Utilities/BinningData.hpp"
#include "Acts/Utilities/Enumerate.hpp"
#include "Acts/Utilities/StringHelpers.hpp"

#include <algorithm>

Acts::Experimental::DetectorComponent::PortalContainer
Acts::Experimental::detail::CubicDetectorHelper::connect(
    const GeometryContext& gctx,
    std::vector<std::shared_ptr<Experimental::DetectorVolume>>& volumes,
    BinningValue bValue, const std::vector<unsigned int>& selectedOnly,
    Acts::Logging::Level logLevel) {
  ACTS_LOCAL_LOGGER(getDefaultLogger("CubicDetectorHelper", logLevel));

  ACTS_DEBUG("Connect " << volumes.size() << " detector volumes in "
                        << binningValueNames()[bValue] << ".");

  // Check transform for consistency
  auto centerDistances =
      ConsistencyChecker::checkCenterAlignment(gctx, volumes, bValue);

  // Assign the portal indices according to the volume bounds definition
  std::array<BinningValue, 3u> possibleValues = {binX, binY, binZ};
  // 1 -> [ 2,3 ] for binX connection (cylclic one step)
  // 2 -> [ 4,5 ] for binY connection (cylclic two steps)
  // 0 -> [ 0,1 ] for binZ connection (to be in line with cylinder covnention)
  using PortalSet = std::array<std::size_t, 2u>;
  std::vector<PortalSet> portalSets = {
      {PortalSet{2, 3}, PortalSet{4, 5}, PortalSet{0, 1}}};

  // This is the picked set for fusing
  auto [wasteIndex, keepIndex] = portalSets[bValue];

  // Log the merge splits, i.e. the boundaries of the volumes
  std::array<std::vector<ActsScalar>, 3u> mergeSplits;
  std::array<ActsScalar, 3u> mergeHalfLengths = {
      0.,
      0.,
      0.,
  };

  // Pick the counter part value
  auto counterPart = [&](BinningValue mValue) -> BinningValue {
    for (auto cValue : possibleValues) {
      if (cValue != mValue and cValue != bValue) {
        return cValue;
      }
    }
    return mValue;
  };

  // Things that can be done without a loop be first/last check
  // Estimate the merge parameters: the scalar and the transform
  using MergeParameters = std::tuple<ActsScalar, Transform3>;
  std::map<std::size_t, MergeParameters> mergeParameters;
  auto& firstVolume = volumes.front();
  auto& lastVolume = volumes.back();
  // Values
  const auto firstBoundValues = firstVolume->volumeBounds().values();
  const auto lastBoundValues = lastVolume->volumeBounds().values();
  Vector3 stepDirection = firstVolume->transform(gctx).rotation().col(bValue);

  for (auto [im, mergeValue] : enumerate(possibleValues)) {
    // Skip the bin value itself, fusing will took care of that
    if (mergeValue == bValue) {
      continue;
    }
    for (auto [is, index] : enumerate(portalSets[mergeValue])) {
      // Take rotation from first volume
      auto rotation = firstVolume->portalPtrs()[index]
                          ->surface()
                          .transform(gctx)
                          .rotation();
      ActsScalar stepDown = firstBoundValues[bValue];
      ActsScalar stepUp = lastBoundValues[bValue];
      // Take translation from first and last volume
      auto translationF = firstVolume->portalPtrs()[index]
                              ->surface()
                              .transform(gctx)
                              .translation();

      auto translationL = lastVolume->portalPtrs()[index]
                              ->surface()
                              .transform(gctx)
                              .translation();

      Vector3 translation = 0.5 * (translationF - stepDown * stepDirection +
                                   translationL + stepUp * stepDirection);

      Transform3 portalTransform = Transform3::Identity();
      portalTransform.prerotate(rotation);
      portalTransform.pretranslate(translation);
      // The half length to be kept
      ActsScalar keepHalfLength = firstBoundValues[counterPart(mergeValue)];
      mergeParameters[index] = MergeParameters(keepHalfLength, portalTransform);
    }
  }

  // Loop over the volumes and fuse the portals, collect the merge information
  for (auto [iv, v] : enumerate(volumes)) {
    // So far works only in a cubioid setup
    if (v->volumeBounds().type() != VolumeBounds::BoundsType::eCuboid) {
      throw std::invalid_argument(
          "CubicDetectorHelper: volume bounds are not cuboid");
    }

    // Loop to fuse the portals along the connection direction (bValue)
    if (iv > 0u) {
      ACTS_VERBOSE("- fuse portals of volume '"
                   << volumes[iv - 1]->name() << "' with volume '" << v->name()
                   << "'.");
      ACTS_VERBOSE("-- keep " << keepIndex << " of first and waste "
                              << wasteIndex << " of second volume.");
      // Fusing the portals of the current volume with the previous one
      auto keepPortal = volumes[iv - 1]->portalPtrs()[keepIndex];
      auto wastePortal = v->portalPtrs()[wasteIndex];
      keepPortal->fuse(wastePortal);
      v->updatePortal(keepPortal, wasteIndex);
    } else {
    }

    // Get the bound values
    auto boundValues = v->volumeBounds().values();
    // Loop to determine the merge bounds, the new transform
    for (auto [im, mergeValue] : enumerate(possibleValues)) {
      // Skip the bin value itself, fusing will took care of that
      if (mergeValue == bValue) {
        continue;
      }
      // Record the merge splits
      mergeSplits[im].push_back(2 * boundValues[bValue]);
      mergeHalfLengths[im] += boundValues[bValue];
    }
  }

  // Loop to create the new portals as portal replacements
  std::vector<PortalReplacement> pReplacements;
  for (auto [im, mergeValue] : enumerate(possibleValues)) {
    // Skip the bin value itself, fusing took care of that
    if (mergeValue == bValue) {
      continue;
    }

    // Create the new RecangleBounds
    // - there are conventions involved, regarding the bounds orientation
    // - This is an anticyclic swap
    bool mergedInX = true;
    switch (bValue) {
      case binZ: {
        mergedInX = (mergeValue == binY);
      } break;
      case binY: {
        mergedInX = (mergeValue == binX);
      } break;
      case binX: {
        mergedInX = (mergeValue == binZ);
      } break;
      default:
        break;
    }

    // The stitch boundarieS for portal pointing
    std::vector<ActsScalar> stitchBoundaries;
    stitchBoundaries.push_back(-mergeHalfLengths[im]);
    for (auto step : mergeSplits[im]) {
      stitchBoundaries.push_back(stitchBoundaries.back() + step);
    }

    for (auto [is, index] : enumerate(portalSets[mergeValue])) {
      // Check if you need to skip due to selections
      if (not selectedOnly.empty() and
          std::find(selectedOnly.begin(), selectedOnly.end(), index) ==
              selectedOnly.end()) {
        continue;
      }

      auto [keepHalfLength, portalTransform] = mergeParameters[index];
      std::shared_ptr<RectangleBounds> portalBounds =
          mergedInX ? std::make_shared<RectangleBounds>(mergeHalfLengths[im],
                                                        keepHalfLength)
                    : std::make_shared<RectangleBounds>(keepHalfLength,
                                                        mergeHalfLengths[im]);
      auto portalSurface =
          Surface::makeShared<PlaneSurface>(portalTransform, portalBounds);
      auto portal = Portal::makeShared(portalSurface);
      // Make the stitch boundaries
      pReplacements.push_back(
          PortalReplacement(portal, index, Direction::Backward,
                            stitchBoundaries, (mergedInX ? binX : binY)));
    }
  }
  // Return proto container
  DetectorComponent::PortalContainer dShell;

  // Update the portals of all volumes
  // Exchange the portals of the volumes
  for (auto& iv : volumes) {
    ACTS_VERBOSE("- update portals of volume '" << iv->name() << "'.");
    for (auto& [p, i, dir, boundaries, binning] : pReplacements) {
      // Fill the map
      dShell[i] = p;
      ACTS_VERBOSE("-- update portal with index " << i);
      iv->updatePortal(p, i);
    }
  }
  // Done.

  return dShell;
}

Acts::Experimental::DetectorComponent::PortalContainer
Acts::Experimental::detail::CubicDetectorHelper::connect(
    const GeometryContext& gctx,
    const std::vector<DetectorComponent::PortalContainer>& containers,
    BinningValue bValue, const std::vector<unsigned int>& selectedOnly,
    Acts::Logging::Level logLevel) noexcept(false) {
  // The local logger
  ACTS_LOCAL_LOGGER(getDefaultLogger("CubicDetectorHelper", logLevel));

  ACTS_DEBUG("Connect " << containers.size() << " containers in "
                        << binningValueNames()[bValue] << ".");

  // Return the new container
  DetectorComponent::PortalContainer dShell;

  // The possible bin values
  std::array<BinningValue, 3u> possibleValues = {binX, binY, binZ};
  // And their associated portal sets, see above
  using PortalSet = std::array<std::size_t, 2u>;
  std::vector<PortalSet> portalSets = {
      {PortalSet{2, 3}, PortalSet{4, 5}, PortalSet{0, 1}}};

  // This is the picked set for refubishing
  auto [startIndex, endIndex] = portalSets[bValue];
  // They are identical to waste and kepp index, for clarity rename them
  size_t wasteIndex = startIndex;
  size_t keepIndex = endIndex;

  // Fusing along the connection direction (bValue)
  for (std::size_t ic = 1; ic < containers.size(); ++ic) {
    auto& formerContainer = containers[ic - 1];
    auto& currentContainer = containers[ic];
    // Check and throw exception
    if (formerContainer.find(keepIndex) == formerContainer.end()) {
      throw std::invalid_argument(
          "CubicDetectorHelper: proto container has no fuse portal at index of "
          "former container.");
    }
    if (currentContainer.find(wasteIndex) == currentContainer.end()) {
      throw std::invalid_argument(
          "CubicDetectorHelper: proto container has no fuse portal at index of "
          "current container.");
    }
    std::shared_ptr<Portal> keepPortal =
        formerContainer.find(keepIndex)->second;
    std::shared_ptr<Portal> wastePortal =
        currentContainer.find(wasteIndex)->second;
    keepPortal->fuse(wastePortal);
    for (auto& av : wastePortal->attachedDetectorVolumes()[1u]) {
      ACTS_VERBOSE("Update portal of detector volume '" << av->name() << "'.");
      av->updatePortal(keepPortal, keepIndex);
    }
  }
  // Proto container refurbishment - outside
  dShell[startIndex] = containers.front().find(startIndex)->second;
  dShell[endIndex] = containers.back().find(endIndex)->second;

  // Create remaining outside shells now
  std::vector<unsigned int> sidePortals = {};
  for (auto sVals : possibleValues) {
    if (sVals != bValue) {
      sidePortals.push_back(portalSets[sVals][0]);
      sidePortals.push_back(portalSets[sVals][1]);
    }
  }

  // Strip the side volumes
  auto sideVolumes =
      PortalHelper::stripSideVolumes(containers, sidePortals, selectedOnly);

  ACTS_VERBOSE("There remain " << sideVolumes.size()
                               << " side volume packs to be connected");
  for (auto [s, volumes] : sideVolumes) {
    ACTS_VERBOSE(" - connect " << volumes.size() << " at selected side " << s);
    auto pR = connect(gctx, volumes, bValue, {s}, logLevel);
    if (pR.find(s) != pR.end()) {
      dShell[s] = pR.find(s)->second;
    }
  }

  // Done.
  return dShell;
}

std::array<std::vector<Acts::ActsScalar>, 3u>
Acts::Experimental::detail::CubicDetectorHelper::xyzBoundaries(
    [[maybe_unused]] const GeometryContext& gctx,
    [[maybe_unused]] const std::vector<
        const Acts::Experimental::DetectorVolume*>& volumes,
    Acts::Logging::Level logLevel) {
  // The local logger
  ACTS_LOCAL_LOGGER(getDefaultLogger("CubicDetectorHelper", logLevel));

  // The return boundaries
  std::array<std::vector<Acts::ActsScalar>, 3u> boundaries;

  // The map for collecting
  std::array<std::map<ActsScalar, size_t>, 3u> valueMaps;
  auto& xMap = valueMaps[0u];
  auto& yMap = valueMaps[1u];
  auto& zMap = valueMaps[2u];

  auto fillMap = [&](std::map<ActsScalar, size_t>& map,
                     const std::array<ActsScalar, 2u>& values) {
    for (auto v : values) {
      if (map.find(v) != map.end()) {
        ++map[v];
      } else {
        map[v] = 1u;
      }
    }
  };

  // Loop over the volumes and collect boundaries
  for (const auto& v : volumes) {
    if (v->volumeBounds().type() == Acts::VolumeBounds::BoundsType::eCuboid) {
      auto bValues = v->volumeBounds().values();
      // The min/max values
      ActsScalar halfX = bValues[CuboidVolumeBounds::BoundValues::eHalfLengthX];
      ActsScalar halfY = bValues[CuboidVolumeBounds::BoundValues::eHalfLengthY];
      ActsScalar halfZ = bValues[CuboidVolumeBounds::BoundValues::eHalfLengthZ];
      // Get the transform @todo use a center of gravity of the detector
      auto translation = v->transform(gctx).translation();
      // The min/max values
      ActsScalar xMin = translation.x() - halfX;
      ActsScalar xMax = translation.x() + halfX;
      ActsScalar yMin = translation.y() - halfY;
      ActsScalar yMax = translation.y() + halfY;
      ActsScalar zMin = translation.z() - halfZ;
      ActsScalar zMax = translation.z() + halfZ;
      // Fill the maps
      fillMap(xMap, {xMin, xMax});
      fillMap(yMap, {yMin, yMax});
      fillMap(zMap, {zMin, zMax});
    }
  }

  for (auto [im, map] : enumerate(valueMaps)) {
    for (auto [key, value] : map) {
      boundaries[im].push_back(key);
    }
    std::sort(boundaries[im].begin(), boundaries[im].end());
  }

  ACTS_VERBOSE("- did yield " << boundaries[0u].size() << " boundaries in X.");
  ACTS_VERBOSE("- did yield " << boundaries[1u].size() << " boundaries in Y.");
  ACTS_VERBOSE("- did yield " << boundaries[2u].size() << " boundaries in Z.");

  return boundaries;
}
