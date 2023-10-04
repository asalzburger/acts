// This file is part of the Acts project.
//
// Copyright (C) 2023 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Detector/DetectorComponents.hpp"
#include "Acts/Geometry/GeometryContext.hpp"

namespace Acts {
namespace Experimental {

/// @brief  This is the interface for detector component builders;
/// such a builder could be a simple detector volume builder, with
/// or without internal structure, or more complicated objects.
///
/// Overall, the detector building can be seen as a tree structure,
/// where the top level builder is the root that branches to the sub
/// builders.
///
/// In order to aid this tree structure, the builder interface allows
/// the definition of branch connections
class IDetectorComponentBuilder {
 public:
  /// @brief Nested branch connection struct
  struct BranchConnection {
    std::string targetName = "";
    std::shared_ptr<IDetectorComponentBuilder> targetBuilder = nullptr;
  };

  virtual ~IDetectorComponentBuilder() = default;

  /// The interface method to be implemented by all detector
  /// component builder
  ///
  /// @param gctx The geometry context for this call
  ///
  /// @return an outgoing detector component
  virtual DetectorComponent construct(const GeometryContext& gctx) const = 0;

  /// Write/read access to parent connections
  BranchConnection& parent() { return m_parent; }

  /// Write/read access to child connections
  BranchConnection& child() { return m_child; }

 private:
  /// The parent connection
  BranchConnection m_parent;
  /// The child connection
  BranchConnection m_child;
};

}  // namespace Experimental
}  // namespace Acts
