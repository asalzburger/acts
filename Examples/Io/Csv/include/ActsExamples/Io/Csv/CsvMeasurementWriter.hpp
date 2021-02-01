// This file is part of the Acts project.
//
// Copyright (C) 2020 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/EventData/Measurement.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/GeometryHierarchyMap.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/Geometry/TrackingGeometry.hpp"
#include "Acts/Utilities/Helpers.hpp"
#include "ActsExamples/Digitization/SmearingAlgorithm.hpp"
#include "ActsExamples/EventData/Cluster.hpp"
#include "ActsExamples/EventData/Index.hpp"
#include "ActsExamples/EventData/Measurement.hpp"
#include "ActsExamples/EventData/SimHit.hpp"
#include "ActsExamples/Framework/WriterT.hpp"

#include <memory>
#include <mutex>
#include <vector>

#include <TTree.h>

class TFile;
class TTree;

namespace ActsExamples {

/// @class CsvMeasurementWriter
///
/// This writes multiples file per event containing information about the
/// measurement, the associated truth information and the cell/channel details
///
///     event000000001-cells.csv
///     event000000001-hits.csv
///     event000000001-truth.csv
///     event000000002-cells.csv
///     event000000002-hits.csv
///     event000000002-truth.csv
///     ...
///
///
/// Safe to use from multiple writer threads - uses a std::mutex lock.
class CsvMeasurementWriter final : public WriterT<MeasurementContainer> {
 public:
  struct Config {
    /// Which measurement collection to write.
    std::string inputMeasurements;
    /// Which cluster collection to write (optional)
    std::string inputClusters;
    /// Which simulated (truth) hits collection to use.
    std::string inputSimHits;
    /// Input collection to map measured hits to simulated hits.
    std::string inputMeasurementSimHitsMap;
    std::string filePath = "";          ///< path of the output file
    std::string fileMode = "RECREATE";  ///< file access mode
    /// The indices for this digitization configurations
    Acts::GeometryHierarchyMap<std::vector<Acts::BoundIndices>> boundIndices;
    /// Tracking geometry required to access local-to-global transforms.
    std::shared_ptr<const Acts::TrackingGeometry> trackingGeometry;
  };

  /// Constructor with
  /// @param cfg configuration struct
  /// @param output logging level
  CsvMeasurementWriter(const Config& cfg, Acts::Logging::Level lvl);

  /// Virtual destructor
  ~CsvMeasurementWriter() final override;

  /// End-of-run hook
  ProcessCode endRun() final override;

 protected:
  /// This implementation holds the actual writing method
  /// and is called by the WriterT<>::write interface
  ///
  /// @param ctx The Algorithm context with per event information
  /// @param measurements is the data to be written out
  ProcessCode writeT(const AlgorithmContext& ctx,
                     const MeasurementContainer& measurements) final override;

 private:
  Config m_cfg;
  std::mutex m_writeMutex;  ///< protect multi-threaded writes

  std::unordered_map<Acts::GeometryIdentifier, const Acts::Surface*>
      m_dSurfaces;  ///< All surfaces that could carry measurements
};

}  // namespace ActsExamples
