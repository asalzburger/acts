// This file is part of the Acts project.
//
// Copyright (C) 2023 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <tuple>
#include <vector>
#include <memory>
#include <string>

namespace Acts {

struct GeoModelTree;
class GeoModelDetectorElement;
class Surface;

/// A factory to convert GeoModel volume into sensitive
/// or passive surfaces which are filled into a Cache object,
/// also the create GeoModelDetectorElements which are also 
/// returned.
class GeoModelDetectorSurfaceFactory {
 public:
  /// Collect the sensitive surface & detector element
  using GeoModelSensitiveSurface =
      std::tuple<std::shared_ptr<GeoModelDetectorElement>,
                 std::shared_ptr<Surface>>;

  /// Collect the passive surfaces, bool whether it should be
  /// added as an "always try, i.e. assignToAll=true" surface
  using GeoModelPassiveSurface = std::tuple<std::shared_ptr<Surface>, bool>;

  /// Nested cache that records the conversion status
  struct Cache {
    /// The created detector elements - for the detector store
    std::vector<GeoModelSensitiveSurface> sensitiveSurfaces;
    /// The created non-const surfaces - for further processing,
    std::vector<GeoModelPassiveSurface> passiveSurfaces;
  };

  /// The options to steer the conversion
  struct Options {
    std::vector<std::string> queries = {};
  };

  /// The GeoModel detector element factory
  ///
  /// @param mlogger a screen output logger
  GeoModelDetectorSurfaceFactory(
      std::unique_ptr<const Logger> mlogger = getDefaultLogger(
          "GeoModelDetectorSurfaceFactory", Acts::Logging::INFO));

  /// Construction method of the detector elements
  ///
  /// @param cache [in,out] into which the Elements are filled
  /// @param gctx the geometry context
  /// @param geoModelTree the gjeo model tree
  /// @param options to steer the conversion
  ///
  /// @note this method will call the recursive construction
  void construct(Cache& cache, const GeometryContext& gctx,
                 const GeoModelTree& geoModelTree, const Options& options);

 private:
  /// Logging instance
  std::unique_ptr<const Logger> m_logger;

  /// Private access to the logger
  const Logger& logger() const { return *m_logger; }
};

}  // namespace Acts
