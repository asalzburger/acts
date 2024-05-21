// This file is part of the Acts project.
//
// Copyright (C) 2019 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Units.hpp"
#include "Acts/EventData/TrackParameters.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "Acts/Utilities/Result.hpp"
#include "Acts/Vertexing/FsmwMode1dFinder.hpp"
#include "Acts/Vertexing/ImpactPointEstimator.hpp"
#include "Acts/Vertexing/Vertex.hpp"
#include "Acts/Vertexing/VertexFitterConcept.hpp"
#include "Acts/Vertexing/VertexingOptions.hpp"

#include <unordered_map>

namespace Acts {

/// @class ZScanVertexFinder
///
/// @brief Implements a vertex finder based on the mode of z0 values:
/// 1. Determines the mode value of all input track z0 values
/// 2. If no constraint is given, returns (0,0, z0_mode) as vertex position
/// 3. If vertex constraint is given with x=x_constr and y=y_constr,
///    the returned vertex position will be (x_constr, y_constr, z0_mode).
template <typename vfitter_t>
class ZScanVertexFinder {
  static_assert(VertexFitterConcept<vfitter_t>,
                "Vertex fitter does not fulfill vertex fitter concept.");
  using Propagator_t = typename vfitter_t::Propagator_t;

 public:
  /// Configuration struct
  struct Config {
    /// @brief Finder configuration
    ///
    /// @param ipEst ImpactPointEstimator
    Config(const ImpactPointEstimator<Propagator_t>& ipEst)
        : ipEstimator(ipEst) {}

    // ImpactPointEstimator
    ImpactPointEstimator<Propagator_t> ipEstimator;

    // FsmwMode1dFinder
    FsmwMode1dFinder mode1dFinder;

    // disables all weights, set all weights to 1.
    bool disableAllWeights = false;
    // constraint parameters
    float constraintcutoff = 9.;
    float constrainttemp = 1.;
    // use LogPt for weighting
    bool useLogPt = true;
    // use pt for weighting
    bool usePt = false;
    // minimum pt
    double minPt = 0.4 * UnitConstants::GeV;
    // exponent used for weighting if usePt
    double expPt = 1.;
    // minimum required weight
    double minWeight = 0.01;

    // Function to extract parameters from InputTrack
    InputTrack::Extractor extractParameters;
  };

  /// State struct for fulfilling interface
  struct State {};

  /// @brief Constructor for user-defined InputTrack type
  ///
  /// @param cfg Configuration object
  /// @param logger Logging instance
  ZScanVertexFinder(const Config& cfg,
                    std::unique_ptr<const Logger> logger =
                        getDefaultLogger("ZScanVertexFinder", Logging::INFO))
      : m_cfg(cfg), m_logger(std::move(logger)) {
    if (!m_cfg.extractParameters.connected()) {
      throw std::invalid_argument(
          "ZScanVertexFinder: "
          "No track parameter extractor provided.");
    }
  }

  /// @brief Function that determines single vertex,
  /// based on z0 values of input tracks,
  /// using a Half Sample Mode algorithm
  ///
  /// @param trackVector Input track collection
  /// @param vertexingOptions Vertexing options
  /// @param state State for fulfilling correct interface
  ///
  /// @return Vector of vertices, filled with a single
  ///         vertex (for consistent interfaces)
  Result<std::vector<Vertex>> find(const std::vector<InputTrack>& trackVector,
                                   const VertexingOptions& vertexingOptions,
                                   State& state) const;

 private:
  Config m_cfg;

  /// Logging instance
  std::unique_ptr<const Logger> m_logger;

  /// Private access to logging instance
  const Logger& logger() const { return *m_logger; }
};

}  // namespace Acts

#include "ZScanVertexFinder.ipp"
