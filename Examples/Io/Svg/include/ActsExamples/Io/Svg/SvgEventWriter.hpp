// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Units.hpp"
#include "Acts/Geometry/Extent.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/Plugins/ActSVG/EventDataSvgConverter.hpp"
#include "Acts/Plugins/ActSVG/SurfaceSvgConverter.hpp"
#include "Acts/Plugins/ActSVG/SvgUtils.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/EventData/SimHit.hpp"
#include "ActsExamples/EventData/SimParticle.hpp"
#include "ActsExamples/Framework/AlgorithmContext.hpp"
#include "ActsExamples/Framework/DataHandle.hpp"
#include "ActsExamples/Framework/IWriter.hpp"
#include "ActsExamples/Framework/ProcessCode.hpp"
#include "ActsExamples/Framework/WhiteBoard.hpp"

#include <mutex>
#include <unordered_map>

namespace Acts {
class Surface;
}

namespace ActsExamples {

/// Write out (customized) event information in Svg format.
///
/// This writes one file per event into the configured output directory. By
/// default it writes to the current working directory. Files are named
/// using the following schema:
///
///     event000000001_view_{i}.svg
///     event000000002_view_{i}.svg
///
/// for {i} in m_cfg.views.
class SvgEventWriter : public IWriter {
 public:
  struct Config {
    std::string name = "SvgEventWriter";

    std::string outputDir = "";
    std::string outputStem = "selection";

    std::vector<std::string> views = {"xy", "zr"};

    // --- Detector layer
    std::unordered_map<Acts::GeometryIdentifier, const Acts::Surface*>
        sensitiveSurfaces;
    Acts::Svg::SurfaceConverter::Options sensitiveViewOptions;
    std::map<std::string, std::vector<Acts::Extent>> sensitiveViewRange;

    // --- Particle layer (used for truth association)
    std::string inputSimParticles;

    // --- Sim hits layer
    std::string inputSimHits;
    double shimHitParticleThreshold = 0.1 * Acts::UnitConstants::GeV;
    double simHitSize = 5.;
    Acts::Svg::Style simHitStyle; // draw stype
    std::map<std::string, std::vector<Acts::Extent>> simHitViewRange;
    bool simHitsOnly = false;
    std::size_t simHitsInterpolatedPoints = 4;

  };

  /// Constructor with arguments
  SvgEventWriter(const Config& cfg,
                 Acts::Logging::Level level = Acts::Logging::INFO);

  /// Virtual destructor
  ~SvgEventWriter() override = default;

  /// Write data from one event.
  ProcessCode write(const AlgorithmContext& context) override;

  /// End-of-run hook
  ProcessCode finalize() override { return ActsExamples::ProcessCode::SUCCESS; }

  std::string name() const override { return m_cfg.name; }

 private:
  /// The surface and an association flag if it is shown in standard view or not
  using SurfaceView =
      std::pair<Acts::Svg::ProtoSurface, std::map<std::string, bool>>;

  std::unique_ptr<const Acts::Logger> m_logger;

  Config m_cfg;

  ReadDataHandle<SimParticleContainer> m_simParticles{this, "InputParticles"};

  ReadDataHandle<SimHitContainer> m_simHits{this, "InputSimHits"};

  std::mutex m_writeMutex;

  const Acts::Logger& logger() const { return *m_logger; }
};

}  // namespace ActsExamples
