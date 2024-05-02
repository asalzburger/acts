// This file is part of the Acts project.
//
// Copyright (C) 2024 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Geometry/GeometryContext.hpp"

#include <memory>

namespace Acts::Experimental {

class Detector;

/// @brief This is the interface for manipulating a non-const detector
class IDetectorManipulator {
 public:
  virtual ~IDetectorManipulator() = default;

  /// The virtual interface definition for detector manipulators
  ///
  /// @param gctx the geometry context
  /// @param detector the detector to be manipulated
  ///
  virtual void apply(const GeometryContext& gctx, Detector& detector) const = 0;
};

}  // namespace Acts::Experimental
