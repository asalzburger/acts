
// This file is part of the Acts project.
//
// Copyright (C) 2023 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "Acts/Detector/ProtoVolumeConverter.hpp"

#include "Acts/Detector/DetectorHelper.hpp"

Acts::Experimental::DetectorVolumeExternals
Acts::Experimental::ConcentricCylinderConverter::create(
    const GeometryContext&) {
  // Get the extent of this volume and translate
  const auto& pvExtent = protoVolume.extent;
  ActsScalar z = pvExtent.medium(binZ);
  Transform3 transform = Transform3::Identity();
  transform.pretranslate(Vector3(0., 0., z));
  // Now the shape
  ActsScalar rI = pvExtent.min(binR);
  ActsScalar rO = pvExtent.max(binR);
  ActsScalar hZ = 0.5 * pvExtent.absRange(binZ);
  ActsScalar hPhi = M_PI;
  ActsScalar aPhi = 0.;
  if (pvExtent.constrains(binPhi)) {
    hPhi = 0.5 * pvExtent.absRange(binPhi);
    aPhi = pvExtent.medium(binPhi);
  }
  // Return a tuple of transform and bounds
  auto bounds = std::make_unique<CylinderVolumeBounds>(rI, rO, hZ, hPhi, aPhi);
  DetectorVolumeExternals rTuple = {std::move(transform), std::move(bounds)};
  return rTuple;
}

void Acts::Experimental::ContainerBlockBuilder::operator()(
    DetectorBlock& dBlock, const GeometryContext& gctx,
    Acts::Logging::Level logLevel) {
  // Screen output
  ACTS_LOCAL_LOGGER(getDefaultLogger(
      "ContainerBlockBuilder [ " + protoVolume.name + " ]", logLevel));

  ACTS_DEBUG("Building container volume '" << protoVolume.name << "'.");
  auto& dVolumes = std::get<DetectorVolumes>(dBlock);
  // Only one dimensional connection so far possible
  if (protoVolume.container.has_value() and
      protoVolume.container.value().constituentBinning.size() == 1u) {
    // Get the container
    const auto& container = protoVolume.container.value();
    // Get the binning value
    BinningValue bValue = container.constituentBinning.front().binvalue;
    // Run the block builders
    std::vector<ProtoContainer> dContainers = {};
    ACTS_VERBOSE(" - this container has " << container.constituentVolumes.size()
                                          << " constituents");
    for (auto& cv : container.constituentVolumes) {
      // Collect the constituent volumes
      DetectorBlock cBlock;
      cv.blockBuilder(cBlock, gctx, logLevel);
      // Collect the detector volumes & container of the constituent
      const auto& cVolumes = std::get<DetectorVolumes>(cBlock);
      auto& cContainer = std::get<ProtoContainer>(cBlock);
      dVolumes.insert(dVolumes.end(), cVolumes.begin(), cVolumes.end());
      // Collect the proto containers
      dContainers.push_back(cContainer);
    }
    auto& dContainer = std::get<ProtoContainer>(dBlock);
    dContainer = connectContainers(gctx, {bValue}, dContainers, logLevel);
    // A succesful return
    return;
  }

  // throw exception
  throw std::invalid_argument(
      "ContainerBlockBuilder: no binning value provided.");
}
