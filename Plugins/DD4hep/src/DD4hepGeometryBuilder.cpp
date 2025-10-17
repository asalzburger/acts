// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsPlugins/DD4hep/DD4hepGeometryBuilder.hpp"

#include "Acts/Geometry/TrackingGeometry.hpp"
#include "ActsPlugins/DD4hep/DD4hepProcessors.hpp"

#include <fstream>

ActsPlugins::DD4hepGeometryBuilder::DD4hepGeometryBuilder(
    const DD4hepGeometryBuilder::Config& config,
    std::unique_ptr<const Acts::Logger> logger)
    : m_cfg(config), m_logger(std::move(logger)) {}

std::shared_ptr<Acts::TrackingGeometry>
ActsPlugins::DD4hepGeometryBuilder::buildTrackingGeometry(
    const Acts::GeometryContext& /*gctx*/) {
  // Count the sensitive volumes
  DD4hepSensitiveCounter sensitiveCounter;
  DD4hepGraphVizPrinter graphPrinter;

  auto tupledProcessor =
      std::make_tuple(std::move(sensitiveCounter), std::move(graphPrinter));

  auto chainedProcessor =
      DD4hepChainedProcessor<DD4hepSensitiveCounter, DD4hepGraphVizPrinter>{
          std::move(tupledProcessor)};

  auto cache = chainedProcessor.generateCache();

  recursiveTraverse(m_cfg.dd4hepSource, chainedProcessor, cache);

  /**
  auto& graphStream =
      std::get<DD4hepGraphVizPrinter>(chainedProcessor.processors).stream;

  std::ofstream gvFile("dd4hep_geometry.gv");
  gvFile << "digraph DD4hepGeometry {\n";
  gvFile << graphStream.str();
  gvFile << "}\n";
  gvFile.close();
  */

  // ACTS_VERBOSE("Number of sensitive volumes in DD4hep description: "
  //              << sensitiveCounter.nSensitiveVolumes);

  return nullptr;
}