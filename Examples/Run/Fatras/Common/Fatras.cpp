// This file is part of the Acts project.
//
// Copyright (C) 2019-2021 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "Fatras.hpp"

#include "ActsExamples/Detector/IBaseDetector.hpp"
#include "ActsExamples/Fatras/FatrasAlgorithm.hpp"
#include "ActsExamples/Framework/RandomNumbers.hpp"
#include "ActsExamples/Framework/Sequencer.hpp"
#include "ActsExamples/Generators/EventGenerator.hpp"
#include "ActsExamples/Geometry/CommonGeometry.hpp"
#include "ActsExamples/Simulation/CommonSimulation.hpp"
#include "ActsExamples/MagneticField/MagneticFieldOptions.hpp"
#include "ActsExamples/Options/CommonOptions.hpp"

#include <memory>
#include <string>

#include <boost/program_options.hpp>

using namespace ActsExamples;


namespace {

// simulation handling

void setupSimulation(
    const ActsExamples::Options::Variables& vars,
    ActsExamples::Sequencer& sequencer,
    std::shared_ptr<const ActsExamples::RandomNumbers> randomNumbers,
    std::shared_ptr<const Acts::TrackingGeometry> trackingGeometry) {

  using namespace ActsExamples::Simulation;

  auto logLevel = Options::readLogLevel(vars);
  auto fatrasCfg = FatrasAlgorithm::readConfig(vars);
  fatrasCfg.inputParticles = kParticlesSelection;
  fatrasCfg.outputParticlesInitial = kParticlesInitial;
  fatrasCfg.outputParticlesFinal = kParticlesFinal;
  fatrasCfg.outputSimHits = kSimHits;
  fatrasCfg.randomNumbers = randomNumbers;
  fatrasCfg.trackingGeometry = trackingGeometry;
  fatrasCfg.magneticField = ActsExamples::Options::readMagneticField(vars);

  sequencer.addAlgorithm(
      std::make_shared<FatrasAlgorithm>(std::move(fatrasCfg), logLevel));
}

}

// fatras main function
int runFatras(int argc, char* argv[],
              std::shared_ptr<ActsExamples::IBaseDetector> detector) {
  
  using namespace ActsExamples;
  using namespace ActsExamples::Simulation;

  // setup and parse options
  auto desc = Options::makeDefaultOptions();
  Options::addSequencerOptions(desc);
  Options::addRandomNumbersOptions(desc);
  addInputOptions(desc);
  Options::addOutputOptions(desc, OutputFormat::Root | OutputFormat::Csv);
  // add general and detector-specific geometry options
  Options::addGeometryOptions(desc);
  detector->addOptions(desc);
  Options::addMaterialOptions(desc);
  Options::addMagneticFieldOptions(desc);
  // algorithm-specific options
  FatrasAlgorithm::addOptions(desc);

  auto vars = Options::parse(desc, argc, argv);
  if (vars.empty()) {
    return EXIT_FAILURE;
  }

  // basic services
  auto randomNumbers =
      std::make_shared<RandomNumbers>(Options::readRandomNumbersConfig(vars));

  // setup sequencer
  Sequencer sequencer(Options::readSequencerConfig(vars));
  // setup detector geometry and material and the magnetic field
  auto [trackingGeometry, contextDecorators] = Geometry::build(vars, *detector);
  for (auto cdr : contextDecorators) {
    sequencer.addContextDecorator(cdr);
  }
  // setup algorithm chain
  setupInput(vars, sequencer, randomNumbers);
  setupSimulation(vars, sequencer, randomNumbers, trackingGeometry);
  setupOutput(vars, sequencer);

  // run the simulation
  return sequencer.run();
}
