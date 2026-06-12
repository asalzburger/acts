// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "ActsExamples/TrackFinding/TrackFindingAlgorithm.hpp"

#include <memory>

namespace Acts {
class TrackingGeometry;
class MagneticFieldProvider;
class Logger;
}  // namespace Acts

namespace ActsExamples {

/// Create a CKF track finder function whose propagator uses the
/// ActsPlugins::Geant4Stepper: track parameter and covariance transport are
/// performed by the Geant4(e) machinery through the Geant4 geometry,
/// including material effects. The filter's own material handling should be
/// switched off in this case (TrackFindingAlgorithm::Config::
/// multipleScattering / energyLoss).
///
/// The Geant4 world is taken from the Geant4 run manager of the job (e.g.
/// set up by the Geant4 simulation); it must be initialized before the first
/// track finding call. This function has to be called *after* the Geant4
/// run manager exists but *before* it initializes its physics, since the
/// Geant4e navigator has to be installed first - in practice: call it after
/// the Geant4 simulation algorithm has been constructed.
///
/// @param trackingGeometry The ACTS tracking geometry for the navigation
/// @param magneticField The magnetic field (must be the field used by Geant4)
/// @param logger The logger instance
/// @param meanEnergyLoss Apply mean energy loss to the track parameters
/// @param covarianceNoise Include multiple scattering and energy loss noise
///        in the covariance transport
/// @return The track finder function
std::shared_ptr<TrackFindingAlgorithm::TrackFinderFunction>
makeGeant4TrackFinderFunction(
    std::shared_ptr<const Acts::TrackingGeometry> trackingGeometry,
    std::shared_ptr<const Acts::MagneticFieldProvider> magneticField,
    const Acts::Logger& logger, bool meanEnergyLoss = true,
    bool covarianceNoise = true);

}  // namespace ActsExamples
