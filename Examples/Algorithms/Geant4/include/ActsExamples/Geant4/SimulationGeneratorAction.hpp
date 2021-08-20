// This file is part of the Acts project.
//
// Copyright (C) 2017-2020 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Utilities/Logger.hpp"

#include <memory>
#include <string>

#include <G4SystemOfUnits.hh>
#include <G4ThreeVector.hh>
#include <G4VUserPrimaryGeneratorAction.hh>
#include <globals.hh>

class G4Event;

namespace ActsExamples {

namespace Geant4 {

/// @class SimulationGeneratorAction
///
/// @brief configures the run from Generated particles
///
/// The SimulationGeneratorAction is the implementation of the Geant4
/// class G4VUserSimulationGeneratorAction. It reads the input particles
/// from the EventStore and simulates evokes the particle gun.
///
class SimulationGeneratorAction final : public G4VUserPrimaryGeneratorAction {
 public:
  struct Config {
    /// The input particle collection
    std::string inputParticles = "";

    /// The number of hits per particle to be expected
    /// @note best to include secondaries for that
    unsigned int reserveHitsPerParticle = 20;
  };

  /// Construct the generator action
  ///
  /// @param cfg the configuration struct
  /// @param logger the ACTS logging instance
  SimulationGeneratorAction(
      const Config& cfg,
      std::unique_ptr<const Acts::Logger> logger = Acts::getDefaultLogger(
          "SimulationGeneratorAction", Acts::Logging::INFO));

  ~SimulationGeneratorAction() final override;

  /// Interface method to generate the primary
  ///
  /// @param anEvent is the event that will be run
  void GeneratePrimaries(G4Event* anEvent) final override;

 protected:
  Config m_cfg;

 private:
  /// Private access method to the logging instance
  const Acts::Logger& logger() const { return *m_logger; }

  /// The looging instance
  std::unique_ptr<const Acts::Logger> m_logger;
};

}  // namespace Geant4
}  // namespace ActsExamples
