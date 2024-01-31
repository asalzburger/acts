// This file is part of the Acts project.
//
// Copyright (C) 2017-2020 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "ActsExamples/MaterialMapping/MaterialMapping.hpp"

#include "ActsExamples/MaterialMapping/IMaterialWriter.hpp"

#include <unordered_map>

namespace ActsExamples {
struct AlgorithmContext;
}  // namespace ActsExamples

ActsExamples::MaterialMapping::MaterialMapping(
    const ActsExamples::MaterialMapping::Config& cfg,
    Acts::Logging::Level level)
    : ActsExamples::IAlgorithm("MaterialMapping", level), m_cfg(cfg) {
  if (m_cfg.materialMapper == nullptr) {
    throw std::invalid_argument("Missing material mapper");
  }
  // Create states per mapper
  m_state = m_cfg.materialMapper->createState();

  // Prepare input/output
  m_inputMaterialTracks.initialize(m_cfg.collection);
  m_outputMaterialTracks.initialize(m_cfg.mappedMaterialCollection);
  m_outputUnmappedMaterialTracks.initialize(m_cfg.unmappedMaterialCollection);

  ACTS_INFO(
      "This algorithm requires inter-event information, run in single-threaded "
      "mode!");
}

ActsExamples::MaterialMapping::~MaterialMapping() {
  // Finalize the material maps
  Acts::DetectorMaterialMaps detectorMaterial =
      m_cfg.materialMapper->finalizeMaps(*m_state);

  // Loop over the available writers and write the maps
  for (auto& imw : m_cfg.materialWriters) {
    imw->writeMaterial(detectorMaterial);
  }
}

ActsExamples::ProcessCode ActsExamples::MaterialMapping::execute(
    const ActsExamples::AlgorithmContext& context) const {

  // Prepare the output collections
  std::unordered_map<std::size_t, Acts::RecordedMaterialTrack>
      outputCollection = {};
  std::unordered_map<std::size_t, Acts::RecordedMaterialTrack>
      outputUnmappedCollection = {};

  // Run the mapping, and record mapped and unmapped 
  for (const auto& [idTrack, mTrack] : m_inputMaterialTracks(context)) {
    auto [ mapped, unmapped ] = m_cfg.materialMapper->mapMaterialTrack(
        *m_state, context.geoContext, context.magFieldContext, mTrack);
    outputCollection.insert({idTrack, mapped});
    outputUnmappedCollection.insert({idTrack, unmapped});
  }

  // Write the collections to the EventStore
  m_outputMaterialTracks(context, std::move(outputCollection));
  m_outputUnmappedMaterialTracks(context, std::move(outputUnmappedCollection));
  return ActsExamples::ProcessCode::SUCCESS;
}
