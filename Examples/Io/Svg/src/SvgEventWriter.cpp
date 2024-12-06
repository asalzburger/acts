// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/Io/Svg/SvgEventWriter.hpp"
#include "ActsExamples/Io/Common/SimHitHelper.hpp"

#include "Acts/Plugins/ActSVG/EventDataSvgConverter.hpp"
#include "ActsExamples/Utilities/Paths.hpp"
#include "Acts/Visualization/Interpolation3D.hpp"

namespace {

/// Check if the surface is visible in the view
///
/// @param pSurface is the proto surface
/// @param view is the view type
/// @param viewRange is the view range
bool isVisible(
    const Acts::Svg::ProtoSurface& pSurface, const std::string& view,
    const std::map<std::string, std::vector<Acts::Extent>>& viewRange) {
  if (!viewRange.count(view)) {
    return true;
  }

  const auto& ranges = viewRange.at(view);
  for (const auto& vertex : pSurface._vertices) {
    for (const auto& range : ranges) {
      if (range.contains(vertex)) {
        return true;
      }
    }
  }
  return false;
}
}  // namespace

ActsExamples::SvgEventWriter::SvgEventWriter(
    const ActsExamples::SvgEventWriter::Config& config,
    Acts::Logging::Level level)
    : m_logger{Acts::getDefaultLogger(config.name, level)}, m_cfg(config) {
  // Sim particles if there
  if (!m_cfg.inputSimParticles.empty()) {
    m_simParticles.initialize(m_cfg.inputSimParticles);
  }

  // Sim hits if there
  if (!m_cfg.inputSimHits.empty()) {
    m_simHits.initialize(m_cfg.inputSimHits);
  }
}

ActsExamples::ProcessCode ActsExamples::SvgEventWriter::write(
    const AlgorithmContext& context) {
  // ensure exclusive access to tree/file while writing
  std::scoped_lock lock(m_writeMutex);

  ACTS_DEBUG(">>Svg: Event Writer called.");
  const auto& gContext = context.geoContext;

  std::map<Acts::GeometryIdentifier, Acts::Svg::ProtoSurface> protoSurfaces;
  for (const auto& [geoID, surface] : m_cfg.sensitiveSurfaces) {
    auto pSurface = Acts::Svg::SurfaceConverter::convert(
        gContext, *surface, m_cfg.sensitiveViewOptions);
    protoSurfaces[geoID] = pSurface;
  }

  // Layer stack per view
  std::map<std::string, std::vector<actsvg::svg::object>> svgViews;

  // ------------------------------------
  // Layer: "detector"
  for (const auto& view : m_cfg.views) {
    actsvg::svg::object svgView;
    svgView._id =
        "event" + std::to_string(context.eventNumber) + "_detector_" + view;
    svgView._tag = "g";
    svgViews[view].push_back(svgView);
  }

  for (const auto& [geoID, pSurface] : protoSurfaces) {
    for (const auto& view : m_cfg.views) {
      if (view == "xy" && isVisible(pSurface, view, m_cfg.sensitiveViewRange)) {
        auto vsurface = Acts::Svg::View::xy(
            pSurface, "sensitive_" + std::to_string(geoID.value()));
        svgViews[view].back().add_object(vsurface);

      } else if (view == "zr" &&
                 isVisible(pSurface, view, m_cfg.sensitiveViewRange)) {
        auto vsurface = Acts::Svg::View::zr(
            pSurface, "sensitive_" + std::to_string(geoID.value()));
        svgViews[view].back().add_object(vsurface);
      } else if (view != "xy" && view != "zr") {
        throw std::invalid_argument("Unknown view type");
      }
    }
  }

  // ------------------------------------
  // Layer: "particles"
  SimParticleContainer simParticles;
  if (!m_cfg.inputSimParticles.empty()) {
    simParticles = m_simParticles(context);
  }

  // ------------------------------------
  // Layer: "sim_hits"
  if (!m_cfg.inputSimHits.empty()) {
    for (const auto& view : m_cfg.views) {
      actsvg::svg::object svgView;
      svgView._id =
          "event" + std::to_string(context.eventNumber) + "_simhits_" + view;
      svgView._tag = "g";
      svgViews[view].push_back(svgView);
    }

    // Get the sim hits
    const auto& simHits = m_simHits(context);
    std::size_t simHitCounter = 0;

    // Only hit plotting
    if (m_cfg.simHitsOnly) {
      // Write data from internal immplementation
      for (const auto& simHit : simHits) {
        double momentum = simHit.momentum4Before().head<3>().norm();
        if (momentum < m_cfg.shimHitParticleThreshold) {
          continue;
        }
        // local simhit information in global coord.
        const Acts::Vector4& globalPos4 = simHit.fourPosition();
        for (const auto& view : m_cfg.views) {
          if (view == "xy") {
            auto simhit = Acts::Svg::EventDataConverter::pointXY(
                globalPos4.head<3>(), m_cfg.simHitSize, m_cfg.simHitStyle,
                simHitCounter);
            svgViews[view].push_back(simhit);
          } else if (view == "zr") {
            auto simhit = Acts::Svg::EventDataConverter::pointZR(
                globalPos4.head<3>(), m_cfg.simHitSize, m_cfg.simHitStyle,
                simHitCounter);
            svgViews[view].push_back(simhit);
          }
          ++simHitCounter;
        }

      }  // end simHit loop
    } else {
      // attach by particle
      auto particleHits = SimHitHelper::associateHitsToParticle(
          simHits, m_cfg.shimHitParticleThreshold, simParticles);
      // Draw loop
      for (auto& [pId, pHits] : particleHits) {
        for (const auto& view : m_cfg.views) {
          if (view == "xy"){
              auto simtraj = Acts::Svg::EventDataConverter::trajectoryXY(
                  pHits, m_cfg.simHitSize, m_cfg.simHitStyle,
                  m_cfg.simHitsInterpolatedPoints, pId);
              svgViews[view].push_back(simtraj);
          } else if (view == "zr"){
              auto simtraj = Acts::Svg::EventDataConverter::trajectoryZR(
                  pHits, m_cfg.simHitSize, m_cfg.simHitStyle,
                  m_cfg.simHitsInterpolatedPoints, pId);
              svgViews[view].push_back(simtraj);
          }

        } // enf of views loop

      }
    }
  }  // end of sim hits

  // Write out the view per event
  for (const auto& view : m_cfg.views) {
    // open per-event file for all simhit components
    std::string pathSimHit = perEventFilepath(
        m_cfg.outputDir, m_cfg.outputStem + "_" + view + ".svg",
        context.eventNumber);
    Acts::Svg::toFile(svgViews[view], pathSimHit);
  }

  // Successfully done
  return ActsExamples::ProcessCode::SUCCESS;
}
