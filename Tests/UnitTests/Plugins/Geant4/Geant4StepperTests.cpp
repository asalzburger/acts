// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Tolerance.hpp"
#include "Acts/Definitions/Units.hpp"
#include "Acts/EventData/BoundTrackParameters.hpp"
#include "Acts/EventData/ParticleHypothesis.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/MagneticField/ConstantBField.hpp"
#include "Acts/MagneticField/MagneticFieldContext.hpp"
#include "Acts/Propagator/DirectNavigator.hpp"
#include "Acts/Propagator/EigenStepper.hpp"
#include "Acts/Propagator/Propagator.hpp"
#include "Acts/Propagator/StepperConcept.hpp"
#include "Acts/Surfaces/CurvilinearSurface.hpp"
#include "ActsPlugins/Geant4/Geant4Stepper.hpp"
#include "ActsTests/CommonHelpers/FloatComparisons.hpp"

#include <memory>
#include <stdexcept>

#include "G4Box.hh"
#include "G4LogicalVolume.hh"
#include "G4NistManager.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"
#include "G4ThreeVector.hh"

using namespace Acts;
using namespace Acts::UnitLiterals;
using namespace ActsPlugins;

static_assert(Acts::Concepts::SingleStepper<Geant4Stepper>);
static_assert(Acts::StepperStateConcept<Geant4Stepper::State>);

namespace ActsTests {

namespace {

/// Build the (single, process-wide) Geant4 world: a 4 m vacuum box with a
/// 2 mm thick silicon slab at x = -1 m
G4VPhysicalVolume* g4WorldVolume() {
  static G4VPhysicalVolume* world = []() {
    auto* nist = G4NistManager::Instance();
    auto* vacuum = nist->FindOrBuildMaterial("G4_Galactic");
    auto* silicon = nist->FindOrBuildMaterial("G4_Si");

    auto* worldS = new G4Box("World", 4. * CLHEP::m, 4. * CLHEP::m,
                             4. * CLHEP::m);
    auto* worldLV = new G4LogicalVolume(worldS, vacuum, "World");
    auto* worldPV = new G4PVPlacement(nullptr, G4ThreeVector(), worldLV,
                                      "World", nullptr, false, 0);

    auto* slabS = new G4Box("Slab", 1. * CLHEP::mm, 1. * CLHEP::m,
                            1. * CLHEP::m);
    auto* slabLV = new G4LogicalVolume(slabS, silicon, "Slab");
    new G4PVPlacement(nullptr, G4ThreeVector(-1. * CLHEP::m, 0., 0.), slabLV,
                      "Slab", worldLV, false, 0);

    return worldPV;
  }();
  return world;
}

/// The shared magnetic field - the Geant4e session is bound to one field
/// instance per process
std::shared_ptr<const MagneticFieldProvider> g4BField() {
  static auto field =
      std::make_shared<ConstantBField>(Vector3(0., 0., 2_T));
  return field;
}

Geant4Stepper makeStepper(bool meanEnergyLoss = true,
                          bool covarianceNoise = true) {
  Geant4Stepper::Config cfg;
  cfg.worldVolume = g4WorldVolume();
  cfg.bField = g4BField();
  cfg.meanEnergyLoss = meanEnergyLoss;
  cfg.covarianceNoise = covarianceNoise;
  return Geant4Stepper(cfg);
}

/// A well conditioned, non-diagonal bound covariance seed
BoundMatrix testCovariance() {
  BoundVector sigma;
  sigma << 0.1_mm, 0.1_mm, 1_mrad, 1_mrad, 0.01 / 1_GeV, 1_ns;
  BoundMatrix corr = BoundMatrix::Identity();
  for (unsigned int i = 0; i < eBoundSize; ++i) {
    for (unsigned int j = 0; j < eBoundSize; ++j) {
      if (i != j) {
        corr(i, j) = 0.1;
      }
    }
  }
  return sigma.asDiagonal() * corr * sigma.asDiagonal();
}

/// Compare the spatial/angular/momentum block of two bound covariances,
/// the time entries are excluded (not transported by Geant4e)
void checkCovariance(const BoundMatrix& test, const BoundMatrix& reference,
                     double relTol) {
  for (unsigned int i = 0; i < eBoundSize; ++i) {
    for (unsigned int j = 0; j < eBoundSize; ++j) {
      if (i == eBoundTime || j == eBoundTime) {
        continue;
      }
      const double scale =
          std::sqrt(reference(i, i) * reference(j, j));
      BOOST_CHECK_MESSAGE(
          std::abs(test(i, j) - reference(i, j)) <= relTol * scale,
          "covariance entry (" << i << ", " << j << ") differs: "
                               << test(i, j) << " vs " << reference(i, j)
                               << " (tolerance " << relTol * scale << ")");
    }
  }
}

const GeometryContext tgContext = GeometryContext::dangerouslyDefaultConstruct();
const MagneticFieldContext mfContext = MagneticFieldContext();

}  // namespace

BOOST_AUTO_TEST_SUITE(Geant4Suite)

/// Test the state initialization and the parameter accessors
BOOST_AUTO_TEST_CASE(Geant4StepperState) {
  const Geant4Stepper stepper = makeStepper();

  Geant4Stepper::Options options(tgContext, mfContext);
  options.maxStepSize = 10_cm;

  const Vector4 pos4(1_mm, 2_mm, 3_mm, 4_ns);
  const Vector3 dir = Vector3(1., 1., 0.2).normalized();
  const double p = 1.5_GeV;
  const double q = -1_e;

  const BoundTrackParameters start = BoundTrackParameters::createCurvilinear(
      pos4, dir, q / p, testCovariance(), ParticleHypothesis::muon());

  Geant4Stepper::State state = stepper.makeState(options);
  stepper.initialize(state, start);

  CHECK_CLOSE_ABS(stepper.position(state), pos4.segment<3>(0), 1e-9);
  CHECK_CLOSE_ABS(stepper.direction(state), dir, 1e-9);
  CHECK_CLOSE_ABS(stepper.time(state), pos4[3], 1e-9);
  CHECK_CLOSE_REL(stepper.qOverP(state), q / p, 1e-9);
  CHECK_CLOSE_REL(stepper.absoluteMomentum(state), p, 1e-9);
  BOOST_CHECK_EQUAL(stepper.charge(state), q);
  BOOST_CHECK(state.covTransport);
  BOOST_CHECK_EQUAL(state.pathAccumulated, 0.);

  // a curvilinear state at the start reproduces the input
  const auto [curvPars, jac, path] = stepper.curvilinearState(state);
  CHECK_CLOSE_ABS(curvPars.position(tgContext), pos4.segment<3>(0), 1e-6);
  BOOST_CHECK(curvPars.covariance().has_value());
  checkCovariance(*curvPars.covariance(), testCovariance(), 1e-6);
  // the time variance is carried through unchanged
  CHECK_CLOSE_REL((*curvPars.covariance())(eBoundTime, eBoundTime),
                  testCovariance()(eBoundTime, eBoundTime), 1e-9);

  // neutral particles are not supported
  const BoundTrackParameters neutral = BoundTrackParameters::createCurvilinear(
      pos4, dir, 1. / p, std::nullopt, ParticleHypothesis::photon());
  Geant4Stepper::State neutralState = stepper.makeState(options);
  BOOST_CHECK_THROW(stepper.initialize(neutralState, neutral),
                    std::invalid_argument);
}

/// Manually step towards a plane surface and check that the constrained
/// step size is honored without a navigator
BOOST_AUTO_TEST_CASE(Geant4StepperManualStepping) {
  const Geant4Stepper stepper = makeStepper();

  Geant4Stepper::Options options(tgContext, mfContext);
  options.maxStepSize = 5_cm;

  const Vector3 dir = Vector3::UnitX();
  const BoundTrackParameters start = BoundTrackParameters::createCurvilinear(
      Vector4::Zero(), dir, -1_e / 1_GeV, std::nullopt,
      ParticleHypothesis::muon());

  Geant4Stepper::State state = stepper.makeState(options);
  stepper.initialize(state, start);

  const auto target =
      CurvilinearSurface(Vector3(30_cm, 0., 0.), Vector3::UnitX()).surface();

  IntersectionStatus status = IntersectionStatus::unreachable;
  for (std::size_t i = 0; i < 1000; ++i) {
    status = stepper.updateSurfaceStatus(
        state, *target, 0, Direction::Forward(),
        BoundaryTolerance::Infinite(), s_onSurfaceTolerance,
        ConstrainedStep::Type::Navigator);
    if (status == IntersectionStatus::onSurface) {
      break;
    }
    BOOST_REQUIRE(status == IntersectionStatus::reachable);
    const auto res = stepper.step(state, Direction::Forward(), nullptr);
    BOOST_REQUIRE(res.ok());
    stepper.releaseStepSize(state, ConstrainedStep::Type::Navigator);
  }
  BOOST_CHECK(status == IntersectionStatus::onSurface);
  BOOST_CHECK_GT(state.nSteps, 1u);

  const auto boundState = stepper.boundState(state, *target);
  BOOST_REQUIRE(boundState.ok());
  const auto& [boundPars, jacobian, pathLength] = *boundState;
  BOOST_CHECK(target
                  ->isOnSurface(tgContext, boundPars.position(tgContext),
                                boundPars.direction())
                  );
  BOOST_CHECK_GT(pathLength, 30_cm);
}

/// Propagate a helix in the vacuum region and compare parameters and
/// covariance against the EigenStepper, then propagate back
BOOST_AUTO_TEST_CASE(Geant4StepperVacuumHelixVsEigen) {
  using G4Propagator = Propagator<Geant4Stepper, DirectNavigator>;
  using EigenPropagator = Propagator<EigenStepper<>, DirectNavigator>;

  const G4Propagator g4Propagator(makeStepper(), DirectNavigator());
  const EigenPropagator eigenPropagator{EigenStepper<>(g4BField()),
                                        DirectNavigator()};

  const Vector3 dir = Vector3(1., 0.3, 0.2).normalized();
  const BoundTrackParameters start = BoundTrackParameters::createCurvilinear(
      Vector4::Zero(), dir, -1_e / 1_GeV, testCovariance(),
      ParticleHypothesis::muon());

  // target plane half a meter along the initial direction
  const auto target = CurvilinearSurface(0.5_m * dir, dir).surface();

  auto makeOptions = [&](const auto& propagator) {
    typename std::decay_t<decltype(propagator)>::template Options<> options(
        tgContext, mfContext);
    options.stepping.maxStepSize = 10_cm;
    options.navigation.externalSurfaces = {target.get()};
    return options;
  };

  const auto g4Result =
      g4Propagator.propagate(start, *target, makeOptions(g4Propagator));
  BOOST_REQUIRE(g4Result.ok());
  BOOST_REQUIRE(g4Result->endParameters.has_value());
  const auto& g4End = *g4Result->endParameters;

  const auto eigenResult =
      eigenPropagator.propagate(start, *target, makeOptions(eigenPropagator));
  BOOST_REQUIRE(eigenResult.ok());
  BOOST_REQUIRE(eigenResult->endParameters.has_value());
  const auto& eigenEnd = *eigenResult->endParameters;

  // parameter agreement
  CHECK_CLOSE_ABS(g4End.position(tgContext), eigenEnd.position(tgContext),
                  10_um);
  CHECK_CLOSE_ABS(g4End.direction(), eigenEnd.direction(), 1e-5);
  CHECK_CLOSE_REL(g4End.absoluteMomentum(), eigenEnd.absoluteMomentum(), 1e-6);
  CHECK_CLOSE_REL(g4End.time(), eigenEnd.time(), 1e-6);
  CHECK_CLOSE_REL(g4Result->pathLength, eigenResult->pathLength, 1e-5);

  // covariance agreement (excluding the time entries)
  BOOST_REQUIRE(g4End.covariance().has_value());
  BOOST_REQUIRE(eigenEnd.covariance().has_value());
  checkCovariance(*g4End.covariance(), *eigenEnd.covariance(), 0.05);

  // propagate backwards to the start surface
  auto backOptions = makeOptions(g4Propagator);
  backOptions.direction = Direction::Backward();
  backOptions.navigation.externalSurfaces = {&start.referenceSurface()};
  const auto backResult = g4Propagator.propagate(
      g4End, start.referenceSurface(), backOptions);
  BOOST_REQUIRE(backResult.ok());
  BOOST_REQUIRE(backResult->endParameters.has_value());
  CHECK_CLOSE_ABS(backResult->endParameters->position(tgContext),
                  start.position(tgContext), 50_um);
  CHECK_CLOSE_ABS(backResult->endParameters->direction(), start.direction(),
                  1e-4);
}

/// Propagate through the silicon slab: material effects inflate the
/// covariance and reduce the momentum; without material effects the result
/// matches the pure field transport of the EigenStepper
BOOST_AUTO_TEST_CASE(Geant4StepperMaterialSlab) {
  using G4Propagator = Propagator<Geant4Stepper, DirectNavigator>;
  using EigenPropagator = Propagator<EigenStepper<>, DirectNavigator>;

  const G4Propagator materialPropagator(
      makeStepper(/*meanEnergyLoss=*/true, /*covarianceNoise=*/true),
      DirectNavigator());
  const G4Propagator cleanPropagator(
      makeStepper(/*meanEnergyLoss=*/false, /*covarianceNoise=*/false),
      DirectNavigator());
  const EigenPropagator eigenPropagator{EigenStepper<>(g4BField()),
                                        DirectNavigator()};

  // trajectory crossing the silicon slab at x = -1 m
  const Vector3 dir = -Vector3::UnitX();
  const BoundTrackParameters start = BoundTrackParameters::createCurvilinear(
      Vector4(-0.5_m, 0., 0., 0.), dir, -1_e / 1_GeV, testCovariance(),
      ParticleHypothesis::muon());
  const auto target =
      CurvilinearSurface(Vector3(-1.4_m, 0., 0.), dir).surface();

  auto makeOptions = [&](const auto& propagator) {
    typename std::decay_t<decltype(propagator)>::template Options<> options(
        tgContext, mfContext);
    options.stepping.maxStepSize = 10_cm;
    options.navigation.externalSurfaces = {target.get()};
    return options;
  };

  const auto materialResult = materialPropagator.propagate(
      start, *target, makeOptions(materialPropagator));
  BOOST_REQUIRE(materialResult.ok());
  BOOST_REQUIRE(materialResult->endParameters.has_value());
  const auto& materialEnd = *materialResult->endParameters;

  const auto cleanResult =
      cleanPropagator.propagate(start, *target, makeOptions(cleanPropagator));
  BOOST_REQUIRE(cleanResult.ok());
  BOOST_REQUIRE(cleanResult->endParameters.has_value());
  const auto& cleanEnd = *cleanResult->endParameters;

  const auto eigenResult =
      eigenPropagator.propagate(start, *target, makeOptions(eigenPropagator));
  BOOST_REQUIRE(eigenResult.ok());
  BOOST_REQUIRE(eigenResult->endParameters.has_value());
  const auto& eigenEnd = *eigenResult->endParameters;

  // mean energy loss reduces the momentum
  BOOST_CHECK_LT(materialEnd.absoluteMomentum(),
                 cleanEnd.absoluteMomentum());

  // material noise inflates the angular and momentum uncertainties
  BOOST_REQUIRE(materialEnd.covariance().has_value());
  BOOST_REQUIRE(cleanEnd.covariance().has_value());
  const auto& materialCov = *materialEnd.covariance();
  const auto& cleanCov = *cleanEnd.covariance();
  BOOST_CHECK_GT(materialCov(eBoundTheta, eBoundTheta),
                 cleanCov(eBoundTheta, eBoundTheta));
  BOOST_CHECK_GT(materialCov(eBoundPhi, eBoundPhi),
                 cleanCov(eBoundPhi, eBoundPhi));

  // without material effects the transport matches the EigenStepper
  CHECK_CLOSE_ABS(cleanEnd.position(tgContext), eigenEnd.position(tgContext),
                  10_um);
  CHECK_CLOSE_ABS(cleanEnd.direction(), eigenEnd.direction(), 1e-5);
  CHECK_CLOSE_REL(cleanEnd.absoluteMomentum(), eigenEnd.absoluteMomentum(),
                  1e-6);
  BOOST_REQUIRE(eigenEnd.covariance().has_value());
  checkCovariance(cleanCov, *eigenEnd.covariance(), 0.05);
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace ActsTests
