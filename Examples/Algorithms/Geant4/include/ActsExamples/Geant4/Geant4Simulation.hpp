// This file is part of the Acts project.
//
// Copyright (C) 2017-2020 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Utilities/Logger.hpp"
#include "Acts/Utilities/PolymorphicValue.hpp"
#include "ActsExamples/Framework/BareAlgorithm.hpp"
#include "ActsExamples/Framework/ProcessCode.hpp"

#include <memory>
#include <mutex>
#include <string>

#include "G4VUserDetectorConstruction.hh"

class G4RunManager;
class G4VUserPrimaryGeneratorAction;
class G4UserRunAction;
class G4UserEventAction;
class G4UserTrackingAction;
class G4UserSteppingAction;
class G4MagneticField;

namespace Acts {
class TrackingGeometry;
class MagneticFieldProvider;
}  // namespace Acts

namespace ActsExamples {

class Geant4SurfaceMapper;

/// Algorithm to run Geant4 simulation in the ActsExamples framework
///
class Geant4Simulation final : public BareAlgorithm {
 public:
  struct Config {
    // Name of the output collection : hits
    std::string outputSimHits = "";

    // Name of the output collection : initial particles
    std::string outputParticlesInitial = "";

    // Name of the output collection : final particles
    std::string outputParticlesFinal = "";

    /// The G4 run manager
    G4RunManager* runManager = nullptr;

    /// User Action: Primary generator action of the simulation
    G4VUserPrimaryGeneratorAction* primaryGeneratorAction = nullptr;

    /// User Action: Run
    G4UserRunAction* runAction = nullptr;

    /// User Action: Event
    G4UserEventAction* eventAction = nullptr;

    /// User Action: Tracking
    G4UserTrackingAction* trackingAction = nullptr;

    /// User Action: Stepping Action
    G4UserSteppingAction* steppingAction = nullptr;

    /// Detector construction object.
    Acts::PolymorphicValue<G4VUserDetectorConstruction> detectorConstruction;

    /// The ACTS Magnetic field provider
    G4MagneticField* magneticField = nullptr;

    /// The ACTS TrackingGeometry
    std::shared_ptr<const Acts::TrackingGeometry> trackingGeometry = nullptr;

    /// A Geant4-Acts surface mapper
    std::shared_ptr<const Geant4SurfaceMapper> g4SurfaceMapper = nullptr;
  };

  Geant4Simulation(Config config,
                   Acts::Logging::Level level = Acts::Logging::INFO);
  ~Geant4Simulation();

  /// Algorithm execute method, called once per event with context
  ///
  /// @param ctx the AlgorithmContext for this event
  ActsExamples::ProcessCode execute(
      const ActsExamples::AlgorithmContext& ctx) const final override;

  /// Readonly access to the configuration
  const Config& config() const { return m_cfg; }

 private:
  Config m_cfg;
  G4RunManager* m_runManager;

  // Has to be mutable; algorithm interface enforces object constness
  mutable std::mutex m_runManagerLock;
};

}  // namespace ActsExamples
