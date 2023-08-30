// This file is part of the Acts project.
//
// Copyright (C) 2022-2023 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Units.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/Plugins/Geant4/Geant4DetectorElement.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <memory>
#include <string>
#include <tuple>
#include <vector>

namespace Acts {
class TrackingGeometry;
class NNbarDetectorElement;
class Surface;

namespace Experimental {
class Detector;
}
}  // namespace Acts

namespace ActsExamples {
class IContextDecorator;

namespace NNbar {

class NNbarDetector {
 public:
  using DetectorElements =
      std::vector<std::shared_ptr<Acts::Geant4DetectorElement>>;
  using DetectorPtr = std::shared_ptr<Acts::Experimental::Detector>;
  using Surfaces = std::vector<std::shared_ptr<Acts::Surface>>;

  using ContextDecorators =
      std::vector<std::shared_ptr<ActsExamples::IContextDecorator>>;
  using TrackingGeometryPtr = std::shared_ptr<const Acts::TrackingGeometry>;

  /// Nested configuration struct
  struct Config {
    /// The detector/geometry name
    std::string name = "";
    /// The Geant4 world volume
    std::string gdmlFile = "";
    /// Inner system
    Acts::ActsScalar innerLayerThickness = 1 * Acts::UnitConstants::mm;
    Acts::ActsScalar innerVolumeEnvelope = 5 * Acts::UnitConstants::mm;
    std::vector<std::string> innerSensitiveMatches = {};
    std::vector<std::string> innerPassiveMatches = {};
    /// TPC system
    Acts::ActsScalar tpcLayerThickness = 0.1 * Acts::UnitConstants::mm;
    Acts::ActsScalar tpcVolumeEnvelope = 5 * Acts::UnitConstants::mm;
    std::vector<std::string> tpcSensitiveMatches = {};
    std::vector<std::string> tpcPassiveMatches = {};
    /// Logging
    Acts::Logging::Level logLevel = Acts::Logging::INFO;
  };

  /// Constructor with @param cfg configuration
  NNbarDetector(const Config& cfg) : m_cfg(cfg) {}

  /// @brief Construct a TrackingGeometry from a Geant4 world volume using the KDTreeTrackingGeometryBuilder builder
  ///
  /// @param cfg the configuration of the Geant4 detector
  /// @param logLevel a logger instance
  ///
  /// @return a tuple of an Acts::TrackingGeometry object,  a ContextDecorator & the created detector elements
  std::tuple<TrackingGeometryPtr, ContextDecorators, DetectorElements>
  constructTrackingGeometry();

 private:
  Config m_cfg;
};

}  // namespace NNbar
}  // namespace ActsExamples
