// This file is part of the Acts project.
//
// Copyright (C) 2024 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Detector/interface/IDetectorManipulator.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <memory>

namespace Acts {

class Surface;

namespace Experimental {

class Detector;

/// @brief A manipulator that fills the gap volumes with the material
///
/// This manipulator finds volumes where the assoiated surfaces would be
/// placed in an assigns them to the volume.
///
/// It will update the candidate search policy to 'findAllPortalsAndSurfaces'
class GapVolumeFiller : public IDetectorManipulator {
 public:
  struct Config {
    /// The surfaces to be filled
    std::vector<std::shared_ptr<Surface>> surfaces;
    /// Auxiliary information
    std::string auxiliary = "";
  };

  /// Constructor
  ///
  /// @param cfg is the configuration struct
  /// @param mlogger logging instance for screen output
  GapVolumeFiller(const Config& cfg,
                  std::unique_ptr<const Logger> mlogger =
                      getDefaultLogger("GapVolumeFiller", Logging::INFO))
      : m_cfg(cfg), m_logger(std::move(mlogger)) {}

  /// Destructor
  ~GapVolumeFiller() override = default;

  /// Fill the gap volumes with the material
  ///
  /// @param gctx the geometry context
  /// @param detector the detector to be manipulated
  void apply(const GeometryContext& gctx, Detector& detector) const override;

 private:
  /// configuration object
  Config m_cfg;

  /// Private access method to the logger
  const Logger& logger() const { return *m_logger; }

  /// logging instance
  std::unique_ptr<const Logger> m_logger;
};

}  // namespace Experimental

}  // namespace Acts
