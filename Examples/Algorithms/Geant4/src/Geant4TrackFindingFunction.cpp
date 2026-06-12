// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/Geant4/Geant4TrackFindingFunction.hpp"

#include "Acts/EventData/TrackContainer.hpp"
#include "Acts/Propagator/Navigator.hpp"
#include "Acts/Propagator/Propagator.hpp"
#include "Acts/TrackFinding/CombinatorialKalmanFilter.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/EventData/Track.hpp"
#include "ActsPlugins/Geant4/Geant4Stepper.hpp"

#include <memory>
#include <stdexcept>
#include <utility>

#include <G4ErrorPropagatorManager.hh>
#include <G4Navigator.hh>
#include <G4TransportationManager.hh>

namespace {

using Stepper = ActsPlugins::Geant4Stepper;
using Navigator = Acts::Navigator;
using Propagator = Acts::Propagator<Stepper, Navigator>;
using CKF =
    Acts::CombinatorialKalmanFilter<Propagator, ActsExamples::TrackContainer>;

struct Geant4TrackFinderFunctionImpl
    : public ActsExamples::TrackFindingAlgorithm::TrackFinderFunction {
  std::shared_ptr<const Acts::TrackingGeometry> trackingGeometry;
  std::shared_ptr<const Acts::MagneticFieldProvider> magneticField;
  std::unique_ptr<const Acts::Logger> logger;
  bool meanEnergyLoss = true;
  bool covarianceNoise = true;

  /// Constructed lazily on the first call: the Geant4 world has to be
  /// initialized (by the Geant4 simulation) before the stepper can be built
  mutable std::unique_ptr<CKF> trackFinder;

  ActsExamples::TrackFindingAlgorithm::TrackFinderResult operator()(
      const ActsExamples::TrackParameters& initialParameters,
      const ActsExamples::TrackFindingAlgorithm::TrackFinderOptions& options,
      ActsExamples::TrackContainer& tracks,
      ActsExamples::TrackProxy rootBranch) const override {
    if (trackFinder == nullptr) {
      G4VPhysicalVolume* world =
          G4TransportationManager::GetTransportationManager()
              ->GetNavigatorForTracking()
              ->GetWorldVolume();
      if (world == nullptr) {
        throw std::runtime_error(
            "makeGeant4TrackFinderFunction: the Geant4 geometry is not "
            "initialized - run a Geant4 simulation in the same job or "
            "initialize a Geant4 run manager before track finding");
      }

      Stepper::Config stepperCfg;
      stepperCfg.worldVolume = world;
      stepperCfg.bField = magneticField;
      stepperCfg.meanEnergyLoss = meanEnergyLoss;
      stepperCfg.covarianceNoise = covarianceNoise;
      Stepper stepper(stepperCfg);

      Navigator::Config navigatorCfg{trackingGeometry};
      navigatorCfg.resolvePassive = false;
      navigatorCfg.resolveMaterial = true;
      navigatorCfg.resolveSensitive = true;
      Navigator navigator(navigatorCfg, logger->cloneWithSuffix("Navigator"));

      Propagator propagator(std::move(stepper), std::move(navigator),
                            logger->cloneWithSuffix("Propagator"));
      trackFinder = std::make_unique<CKF>(std::move(propagator),
                                          logger->cloneWithSuffix("Finder"));
    }

    return trackFinder->findTracks(initialParameters, options, tracks,
                                   rootBranch);
  }
};

}  // namespace

std::shared_ptr<ActsExamples::TrackFindingAlgorithm::TrackFinderFunction>
ActsExamples::makeGeant4TrackFinderFunction(
    std::shared_ptr<const Acts::TrackingGeometry> trackingGeometry,
    std::shared_ptr<const Acts::MagneticFieldProvider> magneticField,
    const Acts::Logger& logger, bool meanEnergyLoss, bool covarianceNoise) {
  // Create the Geant4e propagator manager early: it installs the Geant4e
  // navigator as tracking navigator, which has to happen before the Geant4
  // run manager constructs its physics (the transportation process caches
  // the navigator pointer at construction)
  G4ErrorPropagatorManager::GetErrorPropagatorManager();

  auto impl = std::make_shared<Geant4TrackFinderFunctionImpl>();
  impl->trackingGeometry = std::move(trackingGeometry);
  impl->magneticField = std::move(magneticField);
  impl->logger = logger.clone();
  impl->meanEnergyLoss = meanEnergyLoss;
  impl->covarianceNoise = covarianceNoise;
  return impl;
}
