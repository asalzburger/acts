// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/TrackingGeometry.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <memory>

#include <DD4hep/DetElement.h>

namespace Acts {
class TrackingGeometry;
}

namespace ActsPlugins {

/// Gen3 geometry builder for the DD4hep detector description
class DD4hepGeometryBuilder {
 public:
  // Configuration structure for the DD4hepGeometryBuilder
  struct Config {
    dd4hep::DetElement dd4hepSource;
  };

  /// Constructor for the DD4hepGeometryBuilder
  /// @param config The configuration structure
  /// @param logger Optional logger for the geometry builder
  DD4hepGeometryBuilder(const Config& config,
                        std::unique_ptr<const Acts::Logger> logger =
                            Acts::getDefaultLogger("DD4hepGeometryBuilder",
                                                   Acts::Logging::INFO));

  ~DD4hepGeometryBuilder() = default;

  /// Build the ACTS TrackingGeometry from the DD4hep DetElement
  ///
  /// @param gctx The geometry context at building time
  std::shared_ptr<Acts::TrackingGeometry> buildTrackingGeometry(
      const Acts::GeometryContext& gctx);

 private:
  Config m_cfg;
  std::unique_ptr<const Acts::Logger> m_logger;

  // Access the logger
  const Acts::Logger& logger() const { return *m_logger; }

  /// Traverse the detElement tree recursively and collect compound
  ///
  /// @tparam processor_t The processor type
  ///
  /// @param detElement The DetElement to traverse
  /// @param processor The processor instance
  template <typename processor_t>
  void recursiveTraverse(const dd4hep::DetElement& detElement,
                         processor_t& processor, processor_t::Cache& cache) {
    processor(detElement, cache);
    const dd4hep::DetElement::Children& children = detElement.children();
    for (auto& child : children) {
      dd4hep::DetElement childDetElement = child.second;
      recursiveTraverse(childDetElement, processor, cache);
    }
  }
};

}  // namespace ActsPlugins