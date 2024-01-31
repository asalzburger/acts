// This file is part of the Acts project.
//
// Copyright (C) 2024 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "Acts/Material/SequentialMaterialMapper.hpp"

#include "Acts/Utilities/Enumerate.hpp"

Acts::SequentialMaterialMapper::SequentialMaterialMapper(const Config& cfg)
    : IMaterialMapper(), m_cfg(cfg) {
  // Check if there is at least one mapper
  if (m_cfg.mappers.size() < 2u) {
    throw std::invalid_argument(
        "Minimum of two sequentially chained mappers are required.");
  }
}

std::unique_ptr<Acts::IMaterialMapper::State>
Acts::SequentialMaterialMapper::createState() const {
  auto state = std::make_unique<State>();
  for (auto& mm : m_cfg.mappers) {
    state->mappersAndStates.push_back({mm.get(), mm->createState()});
  }
  // Create the state
  return std::make_unique<State>();
}

std::array<Acts::RecordedMaterialTrack, 2u>
Acts::SequentialMaterialMapper::mapMaterialTrack(
    IMaterialMapper::State& imState, const GeometryContext& gctx,
    const MagneticFieldContext& mctx,
    const RecordedMaterialTrack& mTrack) const {
  // Cast to the actual state and then run sequentially through
  State& mState = static_cast<State&>(imState);

  // Currently mapped and unmapped, updated by each mapper
  std::array<Acts::RecordedMaterialTrack, 2u> current = {
      mTrack, Acts::RecordedMaterialTrack{}};
  // Cummulative mapped and unmapped
  std::array<Acts::RecordedMaterialTrack, 2u> cumulative = {
      Acts::RecordedMaterialTrack{mTrack.first, {}},
      Acts::RecordedMaterialTrack{mTrack.first, {}}};
  // Loop over the mapeprs
  for (auto& [mapper, cache] : mState.mappersAndStates) {
    // Update the current
    current = mapper->mapMaterialTrack(*cache, gctx, mctx, current[0u]);
    for (auto [im, mtrack] : enumerate(cumulative)) {
      // Update the overall mapped material
      mtrack.second.materialInX0 += current[im].second.materialInX0;
      mtrack.second.materialInL0 += current[im].second.materialInL0;
      // Insert the material interactions
      mtrack.second.materialInteractions.insert(
          mtrack.second.materialInteractions.end(),
          current[im].second.materialInteractions.begin(),
          current[im].second.materialInteractions.end());
    }
  }
  // Sort the material tracks at the end
  for (auto& mtrack : cumulative) {
    const Acts::Vector3 vertex = mtrack.first.first;
    auto& rMaterial = mtrack.second;
    std::sort(rMaterial.materialInteractions.begin(),
              rMaterial.materialInteractions.end(),
              [&vertex](const auto& a, const auto& b) {
                return (a.position - vertex).norm() <
                       (b.position - vertex).norm();
              });
  }
  // Return the cumulative output
  return cumulative;
}

Acts::DetectorMaterialMaps Acts::SequentialMaterialMapper::finalizeMaps(
    IMaterialMapper::State& imState) const {
  // Cast to the actual state and run sequentially through
  State& mState = static_cast<State&>(imState);
  Acts::DetectorMaterialMaps detectorMaterial;

  for (auto& [mapper, cache] : mState.mappersAndStates) {
    auto mapperMaterial = mapper->finalizeMaps(*cache);
    // Iterate over the surface material map, check and fill
    for (auto& sMaterial : mapperMaterial.first) {
      if (detectorMaterial.first.count(sMaterial.first)) {
        throw std::invalid_argument(
            "Surface material already exists in the detector material map.");
      }
      detectorMaterial.first[sMaterial.first] = sMaterial.second;
    }
    // Iterate over the volume material maps, check and fill
    for (auto& vMaterial : mapperMaterial.second) {
      if (detectorMaterial.second.count(vMaterial.first)) {
        throw std::invalid_argument(
            "Volume material already exists in the detector material map.");
      }
      detectorMaterial.second[vMaterial.first] = vMaterial.second;
    }
  }
  // Return the detector material
  return detectorMaterial;
}
