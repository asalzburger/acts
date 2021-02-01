// This file is part of the Acts project.
//
// Copyright (C) 2017 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "ActsExamples/Io/Csv/CsvMeasurementWriter.hpp"

#include "Acts/Definitions/Units.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Utilities/Helpers.hpp"
#include "Acts/Utilities/Intersection.hpp"
#include "ActsExamples/EventData/Index.hpp"
#include "ActsExamples/EventData/SimHit.hpp"
#include "ActsExamples/EventData/SimParticle.hpp"
#include "ActsExamples/Framework/WhiteBoard.hpp"
#include "ActsExamples/Utilities/Paths.hpp"
#include "ActsExamples/Utilities/Range.hpp"

#include <ios>
#include <optional>
#include <stdexcept>

#include <TFile.h>
#include <TString.h>

#include "detail/AverageSimHits.hpp"

ActsExamples::CsvMeasurementWriter::CsvMeasurementWriter(
    const ActsExamples::CsvMeasurementWriter::Config& cfg,
    Acts::Logging::Level lvl)
    : WriterT(cfg.inputMeasurements, "CsvMeasurementWriter", lvl), m_cfg(cfg) {
  // Input container for measurements is already checked by base constructor
  if (m_cfg.inputSimHits.empty()) {
    throw std::invalid_argument("Missing simulated hits input collection");
  }
  if (m_cfg.inputMeasurementSimHitsMap.empty()) {
    throw std::invalid_argument(
        "Missing hit-to-simulated-hits map input collection");
  }
  if (not m_cfg.trackingGeometry) {
    throw std::invalid_argument("Missing tracking geometry");
  }
  
}

ActsExamples::CsvMeasurementWriter::~CsvMeasurementWriter() {}

ActsExamples::ProcessCode ActsExamples::CsvMeasurementWriter::endRun() {
  // Write the tree
  return ProcessCode::SUCCESS;
}

ActsExamples::ProcessCode ActsExamples::CsvMeasurementWriter::writeT(
    const AlgorithmContext& ctx, const MeasurementContainer& measurements) {
  const auto& simHits = ctx.eventStore.get<SimHitContainer>(m_cfg.inputSimHits);
  const auto& hitSimHitsMap = ctx.eventStore.get<IndexMultimap<Index>>(
      m_cfg.inputMeasurementSimHitsMap);

  ClusterContainer clusters;
  if (not m_cfg.inputClusters.empty()) {
    clusters = ctx.eventStore.get<ClusterContainer>(m_cfg.inputClusters);
  }

  // Exclusive access to the tree while writing
  std::lock_guard<std::mutex> lock(m_writeMutex);

  
  }

  return ActsExamples::ProcessCode::SUCCESS;
}
