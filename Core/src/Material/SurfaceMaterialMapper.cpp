// This file is part of the Acts project.
//
// Copyright (C) 2024 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "Acts/Material/SurfaceMaterialMapper.hpp"

#include "Acts/Material/detail/TryAllSurfacesPredicter.hpp"
#include "Acts/Material/detail/DistanceAssociaters.hpp"


Acts::SurfaceMaterialMapper::Config::Config(
    const std::vector<const Surface*>& surfaces)
    : predicter(detail::TryAllSurfacesPredicter{surfaces}),
    associater(&detail::closestOrdered) {}

Acts::SurfaceMaterialMapper::SurfaceMaterialMapper(
    const Config& cfg, std::unique_ptr<const Logger> slogger)
    : IMaterialMapper(), m_cfg(cfg), m_logger(std::move(slogger)) {}

std::unique_ptr<Acts::IMaterialMapper::State>
Acts::SurfaceMaterialMapper::createState() const {
  auto state = std::make_unique<State>();

  // Create the state
  return std::make_unique<State>();
}

std::array<Acts::RecordedMaterialTrack, 2u>
Acts::SurfaceMaterialMapper::mapMaterialTrack(
    IMaterialMapper::State& imState, const GeometryContext& gctx,
    const MagneticFieldContext& mctx,
    const RecordedMaterialTrack& mTrack) const {
    
    
  // Cast to the actual state
  State& mState = static_cast<State&>(imState);
  
  // Statistics counter
  mState.nTracks++;
  mState.nSteps += mTrack.second.materialInteractions.size();

  /// Get the surface prediction
  auto prediction =
      m_cfg.predicter(gctx, mctx, mTrack.first.first, mTrack.first.second);
  // Statistics counter
  mState.nIntersections += prediction.size();

  // Return the cumulative output
  return {};
}

Acts::DetectorMaterialMaps Acts::SurfaceMaterialMapper::finalizeMaps(
    IMaterialMapper::State& imState) const {
  State& mState = static_cast<State&>(imState);

  ACTS_INFO("************** Finalizing the material maps ************** ");
  ACTS_INFO("*");
  ACTS_INFO("* Total material tracks    : " << mState.nTracks);
  ACTS_INFO("* Input material steps     : " << mState.nSteps);
  ACTS_INFO("* Projected intersections  : " << mState.nIntersections);

  // Prepare the return object
  Acts::DetectorMaterialMaps detectorMaterial;

  // Return the detector material
  return detectorMaterial;
}
