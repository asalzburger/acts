// This file is part of the Acts project.
//
// Copyright (C) 2021 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Utilities/PolymorphicValue.hpp"
#include "ActsExamples/Framework/Sequencer.hpp"
#include "ActsExamples/Utilities/OptionsFwd.hpp"

#include <memory>

#include <G4VUserDetectorConstruction.hh>

namespace Acts {
class MagneticFieldProvider;
class TrackingGeometry;
}  // namespace Acts

namespace ActsExamples {
namespace Geant4 {

/// Main function for running Geant4 with a specific detector.
///
/// @param argc number of command line arguments
/// @param argv command line arguments
/// @param detector is the detector to be used
/// @param magneticFiels is the magnetic field provided by the job
/// @param trackingGepometry is the (optional) TrackingGeometry
///
/// @note a TrackingGeometry instance is necessary if hit matching
/// is required.
void setupSimulation(
    const ActsExamples::Options::Variables& vars,
    ActsExamples::Sequencer& sequencer,
    Acts::PolymorphicValue<G4VUserDetectorConstruction> detector,
    std::shared_ptr<Acts::MagneticFieldProvider> magneticField = nullptr,
    std::shared_ptr<const Acts::TrackingGeometry> trackingGeometry = nullptr);

}  // namespace Geant4
}  // namespace ActsExamples
