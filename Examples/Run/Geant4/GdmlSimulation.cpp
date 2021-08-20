// This file is part of the Acts project.
//
// Copyright (C) 2021 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "ActsExamples/Geant4/GdmlDetectorConstruction.hpp"
#include "ActsExamples/Geant4/Geant4Options.hpp"
#include "ActsExamples/MagneticField/MagneticFieldOptions.hpp"
#include "ActsExamples/Options/CommonOptions.hpp"
#include "ActsExamples/Simulation/CommonSimulation.hpp"

#include <boost/program_options.hpp>

#include "Geant4.hpp"

int main(int argc, char* argv[]) {
  using namespace ActsExamples;
  using namespace ActsExamples::Simulation;
  using namespace ActsExamples::Geant4;

  // Setup and parse options
  auto desc = Options::makeDefaultOptions();
  Options::addSequencerOptions(desc);
  Options::addInputOptions(desc);
  Options::addOutputOptions(desc, OutputFormat::Root);
  Options::addRandomNumbersOptions(desc);
  Options::addGeant4Options(desc);
  Options::addOutputOptions(desc, OutputFormat::Root | OutputFormat::Csv);
  Options::addMagneticFieldOptions(desc);
  // algorithm-specific options
  desc.add_options()(
      "gdml-file",
      boost::program_options::value<std::string>()->default_value(""),
      "GDML detector file.");

  auto vars = Options::parse(desc, argc, argv);
  if (vars.empty()) {
    return EXIT_FAILURE;
  }
  auto gdmlFile = vars["gdml-file"].as<std::string>();

  // Setup the GDML detector
  Acts::PolymorphicValue<G4VUserDetectorConstruction> g4detector =
      Acts::makePolymorphicValue<GdmlDetectorConstruction>(gdmlFile);

  auto magneticField = ActsExamples::Options::readMagneticField(vars);

  // Basic services
  auto randomNumbers =
      std::make_shared<RandomNumbers>(Options::readRandomNumbersConfig(vars));

  // Setup sequencer
  Sequencer sequencer(Options::readSequencerConfig(vars));

  // Setup algorithm chain: Input / Simulation / Output
  setupInput(vars, sequencer, randomNumbers);
  setupSimulation(vars, sequencer, g4detector);
  setupOutput(vars, sequencer);

  // run the simulation
  return sequencer.run();
}
