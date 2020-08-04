// This file is part of the Acts project.
//
// Copyright (C) 2020 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <functional>
#include <optional>
#include <vector>

#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Utilities/BinUtility.hpp"
#include "Acts/Utilities/Definitions.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "ActsFatras/Digitization/DigitizationCell.hpp"

namespace Acts {
class Surface;
}

namespace ActsFatras {

class PlanarDigitizer {
 public:
  /// @enum The carrier type
  /// which defines the drfit direction
  enum CarrierType : int { eHole = -1, eCharge = 1 };

  /// @class Config
  struct Config {};

  /// Constructor from arguments
  PlanarDigitizer(const Config& cfg,
                  std::unique_ptr<const Acts::Logger> logger =
                      Acts::getDefaultLogger("PlanarDigitizer",
                                             Acts::Logging::INFO))
      : m_cfg(cfg), m_logger(logger.release()) {}

  /// Cell stepping on local surface - cartesian (x-y) grid
  ///
  /// @param bUtility The grid Utility
  /// @param start2D The 2-dim start position (local cartesian coordinates)
  /// @param end2D The 2-dim end position (local cartesian coordinates)
  ///
  /// @return cells on a Cartesian grid, the data field is the projected
  /// length within this cell
  std::vector<DigitizationCell> cellsLocal(
      const Acts::BinUtility& bUtility, const Acts::Vector2D& start2D,
      const Acts::Vector2D& end2D,
      std::function<double(const Eigen::ParametrizedLine<double, 2>&,
                           unsigned int, float)>
          stepper) const;

  /// Digitization method
  std::vector<DigitizationCell> cells(const Acts::GeometryContext& gctx,
                                      const Acts::Vector3D& start,
                                      const Acts::Vector3D& end,
                                      const Acts::Surface& sf,
                                      const Acts::BinUtility& bUtility,
                                      const Acts::Vector3D& drift) const;

  /// Short helper to clip boundary values
  ///
  /// @param boundaries The full boundaries values
  /// @param bs The start bin of the clipping
  /// @param be The end bin of the clipping
  ///
  /// @return The clipped vector
  std::vector<float> clip(const std::vector<float>& boundaries, unsigned int bs,
                          unsigned int be) const;

  /// Mask the local position with the surface bounds
  ///
  /// @param gctx The Geometry context
  /// @param start The Starting position in local
  /// @param end The End position in local coordinates
  /// @param sf The surface
  ///
  /// @return an (optional) pair of 2D vector, std::none means both outside
  std::optional<std::pair<Acts::Vector2D, Acts::Vector2D>> mask(
      const Acts::GeometryContext& gctx, const Acts::Vector2D& start,
      const Acts::Vector2D& end, const Acts::Surface& sf) const;

  /// Digitize method
  /**template <typename module_t, typename bfield_t>
  std::vector<DigitizationCell> digitze(const Acts::GeometryContext& gctx,
                                        const Acts::Vector3D& start,
                                        const Acts::Vector3D& end,
                                        const module_t& module,
                                        const bfield_t& field) const { return
  {}; }
   */
 private:
  /// The Configuration struct
  Config m_cfg;

  /// Logger getter to support macros
  const Acts::Logger& logger() const { return *m_logger; }

  /// Owned logging instance
  std::shared_ptr<const Acts::Logger> m_logger;
};
}  // namespace ActsFatras