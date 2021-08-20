// This file is part of the Acts project.
//
// Copyright (C) 2021 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "Geant4.hpp"

#include "ActsExamples/Detector/IBaseDetector.hpp"
#include "ActsExamples/Framework/Sequencer.hpp"
#include "ActsExamples/Geant4/EventStoreRegistry.hpp"
#include "ActsExamples/Geant4/Geant4Simulation.hpp"
#include "ActsExamples/Geant4/Geant4SurfaceMapper.hpp"
#include "ActsExamples/Geant4/SimulationGeneratorAction.hpp"
#include "ActsExamples/Geant4/SimulationMagneticField.hpp"
#include "ActsExamples/Geant4/SimulationSteppingAction.hpp"
#include "ActsExamples/Geant4/SimulationTrackingAction.hpp"
#include "ActsExamples/MagneticField/MagneticFieldOptions.hpp"
#include "ActsExamples/Options/CommonOptions.hpp"
#include "ActsExamples/Simulation/CommonSimulation.hpp"

#include <memory>
#include <string>

#include <FTFP_BERT.hh>
#include <G4RunManager.hh>
#include <boost/program_options.hpp>

namespace ActsExamples {
namespace Geant4 {

void setupSimulation(
    const Options::Variables& vars, Sequencer& sequencer,
    Acts::PolymorphicValue<G4VUserDetectorConstruction> detector,
    std::shared_ptr<Acts::MagneticFieldProvider> magneticField,
    std::shared_ptr<const Acts::TrackingGeometry> trackingGeometry) {
  // Create an event Store registry
  EventStoreRegistry esRegistry(vars["events"].as<size_t>());

  // The G4 Run Manager and the physics list go first
  auto g4RunManager = new G4RunManager();
  g4RunManager->SetUserInitialization(new FTFP_BERT);

  // The main Geant4 algorithm
  Geant4Simulation::Config g4Cfg;
  g4Cfg.runManager = g4RunManager;

  // The particle generator
  SimulationGeneratorAction::Config g4GenCfg;
  g4GenCfg.inputParticles = Simulation::kParticlesSelection;
  auto g4Generator = new SimulationGeneratorAction(g4GenCfg);

  g4Cfg.primaryGeneratorAction = g4Generator;
  g4Cfg.detectorConstruction = std::move(detector);
  g4Cfg.trackingGeometry = std::move(trackingGeometry);

  // An ACTS Magnetic field is provided
  if (magneticField != nullptr){
    SimulationMagneticField::Config g4FieldCfg;
    g4FieldCfg.magneticField = magneticField;
    g4Cfg.magneticField = new SimulationMagneticField(g4FieldCfg);
  } 


  // An ACTS TrackingGeometry is provided, so simulation for sensitive
  // detectors is turned on - they need to get matched first
  if (g4Cfg.trackingGeometry != nullptr) {
    Geant4SurfaceMapper::Config g4SmCfg;
    g4Cfg.g4SurfaceMapper = std::make_shared<const Geant4SurfaceMapper>(
        g4SmCfg,
        Acts::getDefaultLogger("Geant4SurfaceMapper", Acts::Logging::INFO));

    // The Stepping UserAction for sensitive hits
    SimulationSteppingAction::Config g4StepCfg;
    g4StepCfg.sensitivePrefix = g4SmCfg.mappingPrefix;
    g4Cfg.steppingAction = new SimulationSteppingAction(g4StepCfg);

    // The Tracking User Action for initial / final particle handling
    SimulationTrackingAction::Config g4TrackCfg;
    g4Cfg.trackingAction = new SimulationTrackingAction(g4TrackCfg);

    // Add the output collection
    g4Cfg.outputSimHits = Simulation::kSimHits;
    g4Cfg.outputParticlesInitial = Simulation::kParticlesInitial;
    g4Cfg.outputParticlesFinal = Simulation::kParticlesFinal;

  }

  sequencer.addAlgorithm(std::make_shared<Geant4Simulation>(g4Cfg));

  return;
}

}  // namespace Geant4
}  // namespace ActsExamples
