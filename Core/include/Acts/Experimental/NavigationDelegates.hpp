// This file is part of the Acts project.
//
// Copyright (C) 2021 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Experimental/NavigationState.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Utilities/Delegate.hpp"

#include <memory>
#include <tuple>

/// @note this is foreseen for the 'Geometry' module

namespace Acts {

class Surface;

namespace Experimental {

class Portal;
class DetectorVolume;

/// Base class for all link implementations that need class structure
class INavigationDelegate {
 public:
  virtual ~INavigationDelegate() {}
};

/// Memory managed delegate to guarantee the lifetime
/// of eventual unterlying delegate memory and the
/// delegate function
///
template <typename deletage_type>
struct ManagedDelegate {
 public:
  deletage_type delegate;
  std::shared_ptr<INavigationDelegate> implementation = nullptr;
};

/// Declare a navigation state updator
///
/// This delegate dispatches the local navigation action
/// to a dedicated struct or function that is optimised for
/// the given environment.
///
/// @param nState is the navigation state to be updated
/// @param volume is the volume for which this should be called
/// @param gctx is the current geometry context
/// @param position is the position at the query
/// @param direction is the direction at the query
/// @param absMomentum is the absolute momentum at query
/// @param charge is the charge to be used for the intersection
///
using NavigationStateUpdator = Delegate<void(
    NavigationState& nState, const DetectorVolume& volume,
    const GeometryContext& gctx, const Vector3& position,
    const Vector3& direction, ActsScalar absMomentum, ActsScalar charge)>;

/// Memory  managed navigation state updator
using ManagedNavigationStateUpdator = ManagedDelegate<NavigationStateUpdator>;

/// Declare a Detctor Volume Switching delegate
///
/// @param gctx is the current geometry context
/// @param position is the position at the query
/// @param direction is the direction at the query
///
/// @return the new DetectorVolume into which one changes at this switch
using DetectorVolumeLink = Delegate<const DetectorVolume*(
    const GeometryContext& gctx, const Vector3& position,
    const Vector3& direction)>;

/// Memory managed detector volume link
using ManagedDetectorVolumeLink = ManagedDelegate<DetectorVolumeLink>;

}  // namespace Experimental
}  // namespace Acts
