// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Material/GridSurfaceMaterialAccumulater.hpp"

#include "Acts/Material/ProtoSurfaceMaterial.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Utilities/BinAdjustment.hpp"
#include "Acts/Utilities/BinUtility.hpp"

#include <stdexcept>
#include <utility>

Acts::GridSurfaceMaterialAccumulater::GridSurfaceMaterialAccumulater(
    const Config& cfg, std::unique_ptr<const Logger> mlogger)
    : m_cfg(cfg), m_logger(std::move(mlogger)) {}

std::unique_ptr<Acts::ISurfaceMaterialAccumulater::State>
Acts::GridSurfaceMaterialAccumulater::createState() const {
  auto state = std::make_unique<State>();

  for (const auto& surface : m_cfg.materialSurfaces) {
    GeometryIdentifier geoID = surface->geometryId();
    state->materialSurfaces[geoID] = surface;
  }
  return state;
}

void Acts::GridSurfaceMaterialAccumulater::accumulate(
    ISurfaceMaterialAccumulater::State& state, const GeometryContext& gctx,
    const std::vector<MaterialInteraction>& interactions,
    const std::vector<IAssignmentFinder::SurfaceAssignment>&
        surfacesWithoutAssignment) const {
  State* cState = static_cast<State*>(&state);
  if (cState == nullptr) {
    throw std::invalid_argument(
        "Invalid state object provided, something is seriously wrong.");
  }

  initializeAccumulationState(*cState, gctx);

  using MapBin =
      std::pair<AccumulatedSurfaceMaterial*, std::array<std::size_t, 3>>;
  std::map<AccumulatedSurfaceMaterial*, std::array<std::size_t, 3>>
      touchedMapBins;

  for (const auto& mi : interactions) {
    const Surface* surface = mi.surface;
    GeometryIdentifier geoID = surface->geometryId();
    auto accMaterial = cState->accumulatedMaterial.find(geoID);
    if (accMaterial == cState->accumulatedMaterial.end()) {
      throw std::invalid_argument(
          "Surface material is not found, inconsistent configuration.");
    }

    auto tBin = accMaterial->second.accumulate(mi.intersection, mi.materialSlab,
                                               mi.pathCorrection);
    touchedMapBins.insert(MapBin(&(accMaterial->second), tBin));
  }

  for (const auto& [key, value] : touchedMapBins) {
    std::vector<std::array<std::size_t, 3>> trackBins = {value};
    key->trackAverage(trackBins, true);
  }

  if (m_cfg.emptyBinCorrection) {
    for (const auto& [surface, position, direction] :
         surfacesWithoutAssignment) {
      (void)direction;
      auto missedMaterial =
          cState->accumulatedMaterial.find(surface->geometryId());
      if (missedMaterial == cState->accumulatedMaterial.end()) {
        throw std::invalid_argument(
            "Surface material is not found, inconsistent configuration.");
      }
      missedMaterial->second.trackAverage(position, true);
    }
  }
}

std::map<Acts::GeometryIdentifier,
         std::shared_ptr<const Acts::ISurfaceMaterial>>
Acts::GridSurfaceMaterialAccumulater::finalizeMaterial(
    ISurfaceMaterialAccumulater::State& state,
    const GeometryContext& gctx) const {
  std::map<GeometryIdentifier, std::shared_ptr<const ISurfaceMaterial>>
      sMaterials;

  State* cState = static_cast<State*>(&state);
  if (cState == nullptr) {
    throw std::invalid_argument(
        "Invalid state object provided, something is seriously wrong.");
  }

  if (!cState->accumulationInitialized) {
    initializeAccumulationState(*cState, gctx);
  }

  for (auto& accMaterial : cState->accumulatedMaterial) {
    ACTS_DEBUG("Finalizing map for Surface " << accMaterial.first);
    auto sMaterial = accMaterial.second.totalAverage();
    sMaterials[accMaterial.first] = std::move(sMaterial);
  }

  return sMaterials;
}

void Acts::GridSurfaceMaterialAccumulater::initializeAccumulationState(
    State& state, const GeometryContext& gctx) const {
  if (state.accumulationInitialized) {
    return;
  }

  for (const auto& [geoID, surface] : state.materialSurfaces) {
    const Acts::ISurfaceMaterial* surfaceMaterial = surface->surfaceMaterial();
    if (surfaceMaterial == nullptr) {
      throw std::invalid_argument(
          "Surface material is not set, inconsistent configuration.");
    }

    auto psgm = dynamic_cast<const Acts::ProtoGridSurfaceMaterial*>(
        surfaceMaterial);
    if (psgm == nullptr) {
      throw std::invalid_argument(
          "GridSurfaceMaterialAccumulater expects ProtoGridSurfaceMaterial.");
    }

    Acts::BinUtility binUtility(psgm->binning());
    ACTS_DEBUG("       - (proto) binning from ProtoGridSurfaceMaterial is "
               << binUtility);
    binUtility = Acts::adjustBinUtility(binUtility, *surface, gctx);
    ACTS_DEBUG("       - adjusted binning is " << binUtility);

    state.accumulatedMaterial[geoID] =
        Acts::AccumulatedSurfaceMaterial(std::move(binUtility));
  }

  state.accumulationInitialized = true;
}
