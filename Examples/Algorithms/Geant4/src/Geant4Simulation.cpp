// This file is part of the Acts project.
//
// Copyright (C) 2021 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "ActsExamples/Geant4/Geant4Simulation.hpp"

#include "Acts/Utilities/Helpers.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/Framework/WhiteBoard.hpp"
#include "ActsExamples/Geant4/EventStoreRegistry.hpp"
#include "ActsExamples/Geant4/Geant4SurfaceMapper.hpp"
#include "ActsExamples/Geant4/SimulationGeneratorAction.hpp"

#include <iostream>
#include <stdexcept>

#include <G4RunManager.hh>
#include <G4UserEventAction.hh>
#include <G4UserRunAction.hh>
#include <G4UserSteppingAction.hh>
#include <G4UserTrackingAction.hh>
#include <G4TransportationManager.hh>
#include <G4FieldManager.hh>
#include <G4MagneticField.hh>
#include <G4VUserDetectorConstruction.hh>

using namespace ActsExamples;
using namespace ActsExamples::Geant4;

Geant4Simulation::Geant4Simulation(Geant4Simulation::Config config,
                                   Acts::Logging::Level level)
    : BareAlgorithm("Geant4Simulation", level),
      m_cfg(std::move(config)),
      m_runManager(nullptr) {
  if (m_cfg.outputSimHits.empty() and m_cfg.g4SurfaceMapper != nullptr) {
    throw std::invalid_argument("Missing hit output collection.");
  }

  if (!m_cfg.primaryGeneratorAction) {
    throw std::invalid_argument("Missing G4 PrimaryGeneratorAction object");
  }
  if (!m_cfg.detectorConstruction) {
    throw std::invalid_argument("Missing G4 DetectorConstruction object");
  }
  if (!m_cfg.magneticField) {
    G4FieldManager* fieldMgr =
        G4TransportationManager::GetTransportationManager()->GetFieldManager();
    fieldMgr->SetDetectorField(m_cfg.magneticField);
  }

  m_runManager = config.runManager;
  // Set the detector
  G4VUserDetectorConstruction* g4Detector =
      m_cfg.detectorConstruction.release();
  m_runManager->SetUserInitialization(g4Detector);
  // Set the primary generator action
  m_runManager->SetUserAction(m_cfg.primaryGeneratorAction);
  if (m_cfg.runAction != nullptr) {
    m_runManager->SetUserAction(m_cfg.runAction);
  }
  // Set the user actions
  if (m_cfg.eventAction != nullptr) {
    m_runManager->SetUserAction(m_cfg.eventAction);
  }
  if (m_cfg.trackingAction != nullptr) {
    m_runManager->SetUserAction(m_cfg.trackingAction);
  }
  if (m_cfg.steppingAction != nullptr) {
    m_runManager->SetUserAction(m_cfg.steppingAction);
  }
  m_runManager->Initialize();

  // Map simulation to reconstruction geometry
  // - this is needed if you want to run Geant4 simulation with sensitives
  if (m_cfg.trackingGeometry != nullptr and m_cfg.g4SurfaceMapper != nullptr) {
    ACTS_INFO(
        "Remapping selected volumes from Geant4 to Acts::Surface::GeometryID");

    G4VPhysicalVolume* g4World = g4Detector->Construct();
    int sCounter = 0;
    m_cfg.g4SurfaceMapper->remapSensitiveNames(
        g4World, Acts::Vector3(0., 0., 0.), *m_cfg.trackingGeometry.get(),
        sCounter);

    ACTS_INFO("Remapping successful for " << sCounter << " selected volumes.");
  }
}

Geant4Simulation::~Geant4Simulation() {}

ActsExamples::ProcessCode Geant4Simulation::execute(
    const ActsExamples::AlgorithmContext& ctx) const {
  // Ensure exclusive access to the geant run manager
  std::lock_guard<std::mutex> guard(m_runManagerLock);

  // Register the current event store to the registry
  // this will allow access from the User*Actions
  EventStoreRegistry::boards[ctx.eventNumber] = &(ctx.eventStore);

  // Start simulation. each track is simulated as a separate Geant4 event.
  m_runManager->BeamOn(1);

  // Output handling: Initial/Final particles
  if (not m_cfg.outputParticlesInitial.empty() and
      not m_cfg.outputParticlesFinal.empty()) {
    // Initial state of partciles
    SimParticleContainer outputParticlesInitial;
    outputParticlesInitial.insert(
        EventStoreRegistry::particlesInitial[ctx.eventNumber].begin(),
        EventStoreRegistry::particlesInitial[ctx.eventNumber].end());
    EventStoreRegistry::particlesInitial[ctx.eventNumber].clear();
    // Register to the event store
    ctx.eventStore.add(m_cfg.outputParticlesInitial,
                       std::move(outputParticlesInitial));
    // Final state of partciles
    SimParticleContainer outputParticlesFinal;
    outputParticlesFinal.insert(
        EventStoreRegistry::particlesFinal[ctx.eventNumber].begin(),
        EventStoreRegistry::particlesFinal[ctx.eventNumber].end());
    EventStoreRegistry::particlesFinal[ctx.eventNumber].clear();
    // Register to the event store
    ctx.eventStore.add(m_cfg.outputParticlesFinal,
                       std::move(outputParticlesFinal));
  }

  // Output handling: Simulated hits
  if (not m_cfg.outputSimHits.empty()) {
    SimHitContainer simHits;
    simHits.insert(EventStoreRegistry::hits[ctx.eventNumber].begin(),
                   EventStoreRegistry::hits[ctx.eventNumber].end());
    EventStoreRegistry::hits[ctx.eventNumber].clear();
    // Register to the event store
    ctx.eventStore.add(m_cfg.outputSimHits, std::move(simHits));
  }

  return ActsExamples::ProcessCode::SUCCESS;
}
