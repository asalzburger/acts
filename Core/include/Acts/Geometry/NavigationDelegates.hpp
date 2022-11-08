// This file is part of the Acts project.
//
// Copyright (C) 2021 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/NavigationState.hpp"
#include "Acts/Utilities/Delegate.hpp"

#include <memory>
#include <tuple>

/// @note this is foreseen for the 'Geometry' module

namespace Acts {

class Surface;

namespace Experimental {

class Portal;
class DetectorVolume;
class Detector;

/// Base class for navigaiton delegate implementations
/// that allows to then do some sort of type erasure,
/// e.g. for plotting or I/O.
///
/// The interface is never touched in the navigation stream
class IDelegateImpl {
 public:
  virtual ~IDelegateImpl() {}
};

/// Memory managed delegate to guarantee the lifetime
/// of eventual unterlying delegate memory and the
/// delegate function
///
template <typename deletage_type>
struct ManagedDelegate {
 public:
  deletage_type delegate;
  std::shared_ptr<IDelegateImpl> implementation = nullptr;
};

/// Declare an updator for the local navigation, i.e. the
/// navigation inside a detector volume. This can be called
/// either directly after a volume switch or in order to update
/// within a volume after some progression
///
/// This delegate dispatches the local navigation action
/// to a dedicated struct or function that is optimised for
/// the given environment.
///
/// @param gctx is the current geometry context
/// @param nState [in,out] is the navigation state to be updated
///
/// @note it relies on the detector volume to be set to the state
using SurfaceCandidatesUpdator =
    Delegate<void(const GeometryContext& gctx, NavigationState& nState)>;

/// Memory  managed navigation state updator
using ManagedSurfaceCandidatesUpdator =
    ManagedDelegate<SurfaceCandidatesUpdator>;

/// Declare a Detctor Volume finding or switching delegate
///
/// @param gctx is the current geometry context
/// @param nState [in, out] is the navigation state to be updated
///
/// @return the new DetectorVolume into which one changes at this switch
using DetectorVolumeUpdator =
    Delegate<void(const GeometryContext& gctx, NavigationState& nState)>;

/// Memory managed detector volume link
using ManagedDetectorVolumeUpdator = ManagedDelegate<DetectorVolumeUpdator>;

}  // namespace Experimental
}  // namespace Acts
