// This file is part of the Acts project.
//
// Copyright (C) 2021 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Units.hpp"
#include "Acts/Experimental/DetectorEnvironment.hpp"
#include "Acts/Experimental/DetectorVolume.hpp"
#include "Acts/Propagator/ConstrainedStep.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#include <boost/algorithm/string.hpp>

namespace Acts {

/// Tracer class
///
class Tracer {
 public:
  struct Config {
    /// World volume of this Tracer
    std::shared_ptr<DetectorVolume> world{nullptr};

    // Trial & error navigation
    bool trialAndError = false;
  };

  /// Nested State struct
  ///
  /// It acts as an internal state which is
  /// created for every propagation/extrapolation step
  /// and keep thread-local navigation information
  struct State {
    /// Navigation state: the start surface
    const Surface* startSurface = nullptr;
    /// Navigation state - external state: the current surface
    const Surface* currentSurface = nullptr;
    /// Navigation state: the target surface
    const Surface* targetSurface = nullptr;
    /// Navigation state: the next portal
    const Portal* nextPortal = nullptr;
    /// The surface boundary check -  @todo: move to config
    BoundaryCheck surfaceCheck = true;
    /// The range limit: overstep
    ActsScalar overstepLimit = 0.1;
    /// The range limit: pathLimit
    ActsScalar pathLimit = std::numeric_limits<ActsScalar>::infinity();
    /// Current detector environment
    DetectorEnvironment environment;
    /// The navigation break flag
    bool navigationBreak = false;
    /// The target reached flag
    bool targetReached = false;
  };

  /// Constructor with configuration object
  ///
  /// @param cfg The navigator configuration
  explicit Tracer(Config cfg) : m_cfg{std::move(cfg)} {}

  /// @brief Navigator status call, will be called in two modes
  ///
  /// @tparam propagator_state_t is the type of Propagatgor state
  /// @tparam stepper_t is the used type of the Stepper by the Propagator
  ///
  /// @param [in,out] state is the mutable propagator state object
  /// @param [in] stepper Stepper in use
  template <typename propagator_state_t, typename stepper_t>
  bool status(propagator_state_t& state, const stepper_t& stepper) const {
    // Take the shared logger from the options
    const auto& logger = state.options.logger;

    // Get the position, direction via the stepper
    const Vector3 position = stepper.position(state.stepping);
    const Vector3 direction = stepper.direction(state.stepping);

    // Shorthand the environment
    auto& environment = state.navigation.environment;

    ACTS_DEBUG(volInfo(environment)
               << "Status estimation at position = " << toString(position)
               << " with direction = " << toString(direction));

    // Status check always invalidates current surface
    environment.currentSurface = nullptr;

    // Initialization is not done yet, the volume has to provide it
    if (environment.status == DetectorEnvironment::eUninitialized) {
      // Find the lowest detector volume
      const auto* cVolume =
          m_cfg.world->lowest(state.stepping.geoContext, position);
      // Initialize the environment
      /// @todo: change surfaceCheck to navigationCheck
      environment = cVolume->environment(
          state.stepping.geoContext, position, direction,
          {0., state.navigation.pathLimit}, state.navigation.surfaceCheck,
          m_cfg.trialAndError);
    } else {
      // Analyze the current environement & set currentSurface
      /// @todo set the limit to the path limit from the propagation
      ActsScalar pathLimit = std::numeric_limits<ActsScalar>::infinity();
      handleEnvironment(state, position, direction, {0., pathLimit}, false);
    }
    // Forward the current surface
    state.navigation.currentSurface = environment.currentSurface;
    if (state.navigation.currentSurface) {
      ACTS_DEBUG(volInfo(environment)
                 << "Status estimation resulted in current surface with geoID "
                 << state.navigation.currentSurface->geometryId());
    }

    return true;
  }

  /// @brief Navigator target call
  ///
  /// @tparam propagator_state_t is the type of Propagatgor state
  /// @tparam stepper_t is the used type of the Stepper by the Propagator
  ///
  /// @param [in,out] state is the mutable propagator state object
  /// @param [in] stepper Stepper in use
  ///
  /// @return valid flag
  template <typename propagator_state_t, typename stepper_t>
  bool target(propagator_state_t& state, const stepper_t& stepper) const {
    // Take the shared logger from the options
    const auto& logger = state.options.logger;

    // Shorthand the environment
    auto& environment = state.navigation.environment;

    // Get the position, direction via the stepper
    const Vector3 position = stepper.position(state.stepping);
    const Vector3 direction = stepper.direction(state.stepping);

    ACTS_DEBUG(volInfo(environment)
               << "Target estimation at position = " << toString(position)
               << " with direction = " << toString(direction));

    // Analyze the current environement & set currentSurface
    /// @todo set the overstepLimit given the circumstances
    ActsScalar overstepLimit = -0.1;
    /// @todo set the pathLimit given the propagation restriction
    ActsScalar pathLimit = std::numeric_limits<ActsScalar>::infinity();
    handleEnvironment(state, position, direction, {overstepLimit, pathLimit},
                      true);

    // Set the new step size
    if (environment.status == DetectorEnvironment::eTowardsSurface) {
      ACTS_DEBUG(volInfo(environment)
                 << "Target next Surface at estimated path length = "
                 << environment.surfaces.begin()->intersection.pathLength);
      stepper.updateStepSize(state.stepping, *environment.surfaces.begin(),
                             true);
    } else if (environment.status == DetectorEnvironment::eTowardsPortal) {
      ACTS_DEBUG(volInfo(environment)
                 << "Target next Portal at estimated path length = "
                 << environment.portals.begin()->intersection.pathLength);
      stepper.updateStepSize(state.stepping, *environment.portals.begin(),
                             true);
    } else if (environment.volume == nullptr and
               state.navigation.targetSurface == nullptr) {
      ACTS_DEBUG(volInfo(environment)
                 << "End of world reached. Stop Propagation.");
      state.navigation.targetReached = true;
      return false;
    } else {
      ACTS_WARNING(volInfo(environment)
                   << "Target estimation failed. Break Navigation");
      state.navigation.navigationBreak = true;
      return false;
    }

    // Return to the propagator: true is in valid navigation
    return true;
  }

 private:
  /// Analyze and update the environemnt
  ///
  /// @tparam propagator_state_t is the type of Propagatgor state
  ///
  /// @param state is the propagator state object
  /// @param position is the current position
  /// @param direction is the current direction
  /// @param pathRange is the allowed path range for this call
  /// @param target indicates if this is a target call or not
  template <typename propagator_state_t>
  void handleEnvironment(propagator_state_t& state, const Vector3& position,
                         const Vector3& direction,
                         const std::array<ActsScalar, 2>& pathRange,
                         bool target) const {
    // Take the shared logger from the options
    const auto& logger = state.options.logger;

    // Shorten the names for further readability
    auto& environment = state.navigation.environment;
    auto& surfaces = environment.surfaces;
    // Check the surface candidates
    ActsScalar nextDistance = std::numeric_limits<ActsScalar>::infinity();
    if (not surfaces.empty()) {
      ACTS_DEBUG(volInfo(environment)
                 << "With " << surfaces.size() << " surface candidates.");
      size_t nerase = 0;
      for (auto& s : surfaces) {
        // Update the intersection
        s = s.object->intersect(state.stepping.geoContext, position, direction,
                                state.navigation.surfaceCheck);
        // Swap if two solutions are present and in conflict with pathRange
        if (s.alternative.status == Intersection3D::Status::reachable and
            s.intersection.pathLength < pathRange[0] and
            s.alternative.pathLength > pathRange[0]) {
          s.swapSolutions();
        }
        // We are on surface - ignore for target calls
        if (std::abs(s.intersection.pathLength) < s_onSurfaceTolerance and
            not target) {
          environment.currentSurface = s.object;
          environment.status = DetectorEnvironment::eOnSurface;
          ACTS_DEBUG(volInfo(environment)
                     << "On surface (" << s.object->geometryId()
                     << "), setting current surface.");
          nextDistance = 0.;
          ++nerase;
          break;
        } else if (s.intersection.pathLength >
                       state.navigation.overstepLimit and
                   s.intersection.status == Intersection3D::Status::reachable) {
          nextDistance = s.intersection.pathLength;
          environment.status = DetectorEnvironment::eTowardsSurface;
          break;
        } else {
          environment.status = DetectorEnvironment::eUninitialized;
          ++nerase;
        }
      }
      // Erase the hit and missed ones
      surfaces.erase(surfaces.begin(), surfaces.begin() + nerase);
    }
    // Now handle the portals
    auto& portals = environment.portals;
    if (not portals.empty()) {
      ACTS_DEBUG(volInfo(environment)
                 << "With " << portals.size() << " portal candidates.");
      for (auto& p : portals) {
        // Update the intersection
        p = p.object->intersect(state.stepping.geoContext, position, direction);
        // On portal triggers a next environment search
        if (std::abs(p.intersection.pathLength) < s_onSurfaceTolerance) {
          ACTS_DEBUG(
              volInfo(environment)
              << "On portal (" << p.object->surfaceRepresentation().geometryId()
              << "), triggering environment update & setting current surface.");
          environment = p.object->next(state.stepping.geoContext, position,
                                       direction, state.navigation.surfaceCheck,
                                       m_cfg.trialAndError);

          ACTS_DEBUG(volInfo(environment) << "Switched to new volume");
          return;
        } else if (p.intersection.pathLength >
                       state.navigation.overstepLimit and
                   p.intersection.status == Intersection3D::Status::reachable) {
          // Reachable portal might overrule reachable surface (if closer)
          if (p.intersection.pathLength < nextDistance) {
            nextDistance = p.intersection.pathLength;
            environment.status = DetectorEnvironment::eTowardsPortal;
          }
          break;
        }
      }
    }
  }

  /// Private helper method to get volume information
  ///
  /// @param environment the detector state object
  ///
  /// @return a string for the volume
  std::string volInfo(const DetectorEnvironment& environment) const {
    return (environment.volume ? environment.volume->name() : "No Volume") +
           " | ";
  }

  Config m_cfg;
};

}  // namespace Acts
