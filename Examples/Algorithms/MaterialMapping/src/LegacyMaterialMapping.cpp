// This file is part of the Acts project.
//
// Copyright (C) 2017-2020 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "ActsExamples/MaterialMapping/LegacyMaterialMapping.hpp"

#include "Acts/Material/AccumulatedMaterialSlab.hpp"
#include "Acts/Material/AccumulatedSurfaceMaterial.hpp"
#include "ActsExamples/MaterialMapping/IMaterialWriter.hpp"

#include <iostream>
#include <stdexcept>
#include <tuple>
#include <unordered_map>

namespace ActsExamples {
struct AlgorithmContext;
}  // namespace ActsExamples

ActsExamples::LegacyMaterialMapping::LegacyMaterialMapping(
    const ActsExamples::LegacyMaterialMapping::Config& cfg,
    Acts::Logging::Level level)
    : ActsExamples::IAlgorithm("LegacyMaterialMapping", level), m_cfg(cfg) {
  if (!m_cfg.materialSurfaceMapper && !m_cfg.materialVolumeMapper) {
    throw std::invalid_argument("Missing material mapper");
  } else if (!m_cfg.trackingGeometry) {
    throw std::invalid_argument("Missing tracking geometry");
  }

  m_inputMaterialTracks.initialize(m_cfg.collection);
  m_outputMaterialTracks.initialize(m_cfg.mappingMaterialCollection);

  ACTS_INFO("This algorithm requires inter-event information, "
            << "run in single-threaded mode!");

  if (m_cfg.materialSurfaceMapper) {
    // Generate and retrieve the central cache object
    m_mappingState = m_cfg.materialSurfaceMapper->createState();
  }
  if (m_cfg.materialVolumeMapper) {
    // Generate and retrieve the central cache object
    m_mappingStateVol = m_cfg.materialVolumeMapper->createState();
  }
}

ActsExamples::LegacyMaterialMapping::~LegacyMaterialMapping() {
  Acts::DetectorMaterialMaps detectorMaterial;

  if (m_cfg.materialSurfaceMapper != nullptr &&
      m_cfg.materialVolumeMapper != nullptr) {
    // Finalize all the maps using the cached state
    auto surfaceDetectorMaterial =
        m_cfg.materialSurfaceMapper->finalizeMaps(*m_mappingState);
    auto volumeDetectorMaterial =
        m_cfg.materialVolumeMapper->finalizeMaps(*m_mappingStateVol);
    // Loop over the state, and collect the maps for surfaces
    for (auto& [key, value] : surfaceDetectorMaterial.first) {
      detectorMaterial.first.insert({key, std::move(value)});
    }
    // Loop over the state, and collect the maps for volumes
    for (auto& [key, value] : volumeDetectorMaterial.second) {
      detectorMaterial.second.insert({key, std::move(value)});
    }
  } else if (m_cfg.materialSurfaceMapper != nullptr) {
    detectorMaterial =
        m_cfg.materialSurfaceMapper->finalizeMaps(*m_mappingState);
  } else if (m_cfg.materialVolumeMapper != nullptr) {
    detectorMaterial =
        m_cfg.materialVolumeMapper->finalizeMaps(*m_mappingStateVol);
  }
  // Loop over the available writers and write the maps
  for (auto& imw : m_cfg.materialWriters) {
    imw->writeMaterial(detectorMaterial);
  }
}

ActsExamples::ProcessCode ActsExamples::LegacyMaterialMapping::execute(
    const ActsExamples::AlgorithmContext& context) const {
  // Take the collection from the EventStore
  std::unordered_map<std::size_t, Acts::RecordedMaterialTrack> inputTracks =
      m_inputMaterialTracks(context);

  // Map with its cache
  using MapperCache =
      std::tuple<const Acts::IMaterialMapper*, Acts::IMaterialMapper::State*>;

  std::vector<const MapperCache> mappersCache = {};
  if (m_cfg.materialSurfaceMapper) {
    mappersCache.push_back(
        {m_cfg.materialSurfaceMapper.get(), m_mappingState.get()});
  }
  if (m_cfg.materialVolumeMapper) {
    mappersCache.push_back(
        {m_cfg.materialVolumeMapper.get(), m_mappingStateVol.get()});
  }

  std::unordered_map<std::size_t, Acts::RecordedMaterialTrack>
      outputCollection = {};

  // To make it work with the framework needs a lock guard
  for (const auto& [idTrack, mTrack] : inputTracks) {
    Acts::RecordedMaterialTrack rTrack = mTrack;
    for (auto& [mapper, cache] : mappersCache) {
      mapper->mapMaterialTrack(*cache, context.geoContext,
                               context.magFieldContext, rTrack);
    }
    outputCollection.insert({idTrack, rTrack});
  }
  // Write the collection to the EventStore
  m_outputMaterialTracks(context, std::move(outputCollection));
  return ActsExamples::ProcessCode::SUCCESS;
}

std::vector<std::pair<double, int>>
ActsExamples::LegacyMaterialMapping::scoringParameters(uint64_t surfaceID) {
  std::vector<std::pair<double, int>> scoringParameters;

  if (m_cfg.materialSurfaceMapper) {
    Acts::LegacySurfaceMaterialMapper::State* smState =
        static_cast<Acts::LegacySurfaceMaterialMapper::State*>(m_mappingState.get());

    auto surfaceAccumulatedMaterial =
        smState->accumulatedMaterial.find(Acts::GeometryIdentifier(surfaceID));

    if (surfaceAccumulatedMaterial != smState->accumulatedMaterial.end()) {
      auto matrixMaterial =
          surfaceAccumulatedMaterial->second.accumulatedMaterial();
      for (const auto& vectorMaterial : matrixMaterial) {
        for (const auto& AccumulatedMaterial : vectorMaterial) {
          auto totalVariance = AccumulatedMaterial.totalVariance();
          scoringParameters.push_back(
              {totalVariance.first, totalVariance.second});
        }
      }
    }
  }
  return scoringParameters;
}
