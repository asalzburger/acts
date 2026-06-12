// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsPlugins/Geant4/Geant4Stepper.hpp"

#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/EventData/TransformationHelpers.hpp"
#include "Acts/Propagator/detail/JacobianEngine.hpp"
#include "Acts/Surfaces/CurvilinearSurface.hpp"
#include "ActsPlugins/Geant4/Geant4StepperError.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <CLHEP/Units/SystemOfUnits.h>
#include <G4ChordFinder.hh>
#include <G4ClassicalRK4.hh>
#include <G4ErrorEnergyLoss.hh>
#include <G4ErrorFreeTrajState.hh>
#include <G4ErrorMag_UsualEqRhs.hh>
#include <G4ErrorPlaneSurfaceTarget.hh>
#include <G4ErrorPropagatorData.hh>
#include <G4ErrorPropagatorManager.hh>
#include <G4ErrorStepLengthLimitProcess.hh>
#include <G4EventManager.hh>
#include <G4FieldManager.hh>
#include <G4LogicalVolume.hh>
#include <G4MagneticField.hh>
#include <G4ParticleDefinition.hh>
#include <G4ParticleTable.hh>
#include <G4ProcessManager.hh>
#include <G4ProcessVector.hh>
#include <G4RunManager.hh>
#include <G4StateManager.hh>
#include <G4SteppingManager.hh>
#include <G4Track.hh>
#include <G4TrackingManager.hh>
#include <G4TransportationManager.hh>
#include <G4UImanager.hh>

namespace ActsPlugins {

namespace {

/// Conversion factors between Geant4 (CLHEP) and ACTS unit systems
constexpr double kLengthG4ToActs = Acts::UnitConstants::mm / CLHEP::mm;
constexpr double kEnergyG4ToActs = Acts::UnitConstants::MeV / CLHEP::MeV;
constexpr double kFieldActsToG4 = CLHEP::tesla / Acts::UnitConstants::T;

/// The Geant4e error matrix is transported in GeV and cm units (see
/// G4ErrorFreeTrajState::PropagateError), independent of the CLHEP system
constexpr double kErrLengthG4eToActs = Acts::UnitConstants::cm;
constexpr double kErrInvPG4eToActs = 1. / Acts::UnitConstants::GeV;

/// Convert a 5x5 Geant4e matrix to Eigen
template <typename g4_matrix_t>
Acts::SquareMatrix<5> toEigen(const g4_matrix_t& matrix) {
  Acts::SquareMatrix<5> result;
  for (int i = 0; i < 5; ++i) {
    for (int j = 0; j < 5; ++j) {
      result(i, j) = matrix[i][j];
    }
  }
  return result;
}

/// Projection matrix from the Geant4e free trajectory parameters
/// (1/p, lambda, phi, yPerp, zPerp) to the ACTS curvilinear bound parameters
/// (loc0, loc1, phi, theta, q/p, t) at the physical track direction.
///
/// The Geant4e transverse frame (see G4ErrorFreeTrajState::PropagateError)
/// is built from the Geant4 track momentum direction exactly like the ACTS
/// curvilinear frame (U = z x T normalized, V = T x U). In backward mode the
/// Geant4 track carries the reversed momentum, which flips the lambda and
/// yPerp axes with respect to the physical direction.
///
/// @param charge The physical particle charge
/// @param reversed Whether the Geant4 track momentum is reversed
Eigen::Matrix<double, Acts::eBoundSize, 5> g4eToCurvilinearProjection(
    double charge, bool reversed) {
  Eigen::Matrix<double, Acts::eBoundSize, 5> projection =
      Eigen::Matrix<double, Acts::eBoundSize, 5>::Zero();
  const double flip = reversed ? -1. : 1.;
  projection(Acts::eBoundLoc0, 3) = flip * kErrLengthG4eToActs;
  projection(Acts::eBoundLoc1, 4) = kErrLengthG4eToActs;
  projection(Acts::eBoundPhi, 2) = 1.;
  // theta = pi/2 - lambda
  projection(Acts::eBoundTheta, 1) = -flip;
  projection(Acts::eBoundQOverP, 0) = charge * kErrInvPG4eToActs;
  return projection;
}

/// Convert the Geant4e error matrix to an ACTS curvilinear bound covariance
/// at the physical track direction. The time variance is not transported by
/// Geant4e and is re-inserted from @p timeVariance, time correlations are
/// dropped.
Acts::BoundMatrix g4eCovToCurvilinear(const G4ErrorTrajErr& error,
                                      double charge, bool reversed,
                                      double timeVariance) {
  const auto projection = g4eToCurvilinearProjection(charge, reversed);
  Acts::BoundMatrix cov =
      projection * toEigen(error) * projection.transpose();
  cov(Acts::eBoundTime, Acts::eBoundTime) = timeVariance;
  return cov;
}

/// Convert an ACTS curvilinear bound covariance at the physical track
/// direction to a Geant4e error matrix (inverse of @c g4eCovToCurvilinear,
/// dropping the time rows)
G4ErrorTrajErr curvilinearCovToG4e(const Acts::BoundMatrix& cov, double charge,
                                   bool reversed) {
  Eigen::Matrix<double, 5, Acts::eBoundSize> projection =
      Eigen::Matrix<double, 5, Acts::eBoundSize>::Zero();
  const double flip = reversed ? -1. : 1.;
  projection(3, Acts::eBoundLoc0) = flip / kErrLengthG4eToActs;
  projection(4, Acts::eBoundLoc1) = 1. / kErrLengthG4eToActs;
  projection(2, Acts::eBoundPhi) = 1.;
  projection(1, Acts::eBoundTheta) = -flip;
  projection(0, Acts::eBoundQOverP) = 1. / (charge * kErrInvPG4eToActs);
  const Acts::SquareMatrix<5> converted =
      projection * cov * projection.transpose();
  G4ErrorTrajErr error(5, 0);
  for (int i = 0; i < 5; ++i) {
    for (int j = 0; j <= i; ++j) {
      error[i][j] = converted(i, j);
    }
  }
  return error;
}

/// Flip the Geant4e error matrix between the forward and the reversed
/// momentum frame (lambda and yPerp change sign)
G4ErrorTrajErr flipG4eFrame(const G4ErrorTrajErr& error) {
  constexpr std::array<double, 5> flip = {1., -1., 1., -1., 1.};
  G4ErrorTrajErr flipped(5, 0);
  for (int i = 0; i < 5; ++i) {
    for (int j = 0; j <= i; ++j) {
      flipped[i][j] = flip[i] * flip[j] * error[i][j];
    }
  }
  return flipped;
}

/// Find the Geant4 particle definition matching the given hypothesis and
/// charge, throws if not available in the Geant4e physics list
const G4ParticleDefinition& findG4Particle(
    const Acts::ParticleHypothesis& hypothesis, double charge) {
  if (charge == 0.) {
    throw std::invalid_argument(
        "Geant4Stepper: only charged particles are supported");
  }
  const int absPdg = static_cast<int>(hypothesis.absolutePdg());
  // leptons have a positive PDG code for the negative-charge state
  const bool isLepton = (absPdg >= 11 && absPdg <= 18);
  const int signedPdg =
      (isLepton == (charge < 0.)) ? absPdg : -absPdg;
  G4ParticleDefinition* particle =
      G4ParticleTable::GetParticleTable()->FindParticle(signedPdg);
  if (particle == nullptr) {
    throw std::invalid_argument(
        "Geant4Stepper: no Geant4 particle for PDG code " +
        std::to_string(signedPdg));
  }
  if (particle->GetPDGCharge() * charge < 0.) {
    throw std::invalid_argument(
        "Geant4Stepper: charge mismatch for PDG code " +
        std::to_string(signedPdg));
  }
  return *particle;
}

/// Path length derivatives of the free parameters, needed for the
/// curvilinear projection corrections in the jacobian engine
Acts::FreeVector freeToPathDerivatives(
    const Acts::FreeVector& pars,
    const Acts::ParticleHypothesis& hypothesis, const Acts::Vector3& bField) {
  const Acts::Vector3 direction = pars.segment<3>(Acts::eFreeDir0);
  const double qOverP = pars[Acts::eFreeQOverP];
  const double momentum = hypothesis.extractMomentum(qOverP);
  Acts::FreeVector derivatives = Acts::FreeVector::Zero();
  derivatives.segment<3>(Acts::eFreePos0) = direction;
  derivatives[Acts::eFreeTime] = std::hypot(1., hypothesis.mass() / momentum);
  derivatives.segment<3>(Acts::eFreeDir0) = qOverP * direction.cross(bField);
  return derivatives;
}

}  // namespace

namespace detail {

/// Adapter exposing an Acts::MagneticFieldProvider as a G4MagneticField
class Geant4FieldAdapter final : public G4MagneticField {
 public:
  explicit Geant4FieldAdapter(
      std::shared_ptr<const Acts::MagneticFieldProvider> field)
      : m_field(std::move(field)),
        m_fieldCache(m_field->makeCache(Acts::MagneticFieldContext())) {}

  void GetFieldValue(const G4double point[4], G4double* bField) const final {
    const auto fieldRes = m_field->getField(
        {kLengthG4ToActs * point[0], kLengthG4ToActs * point[1],
         kLengthG4ToActs * point[2]},
        m_fieldCache);
    const Acts::Vector3 field =
        fieldRes.ok() ? *fieldRes : Acts::Vector3::Zero();
    bField[0] = kFieldActsToG4 * field[0];
    bField[1] = kFieldActsToG4 * field[1];
    bField[2] = kFieldActsToG4 * field[2];
  }

 private:
  std::shared_ptr<const Acts::MagneticFieldProvider> m_field;
  mutable Acts::MagneticFieldProvider::Cache m_fieldCache;
};

/// Process-wide Geant4e session. Geant4(e) is built around global state
/// (G4ErrorPropagatorManager, G4TransportationManager, ...), so there can
/// only be one configuration per process.
///
/// Two modes of operation:
///  - standalone: no `G4RunManager` exists, the session owns the full
///    Geant4(e) setup: world, field and the G4ErrorPhysicsList.
///  - attach: a `G4RunManager` exists (e.g. a Geant4 simulation in the same
///    job); the session shares its kernel, world, field and physics list,
///    and shields the simulation from the error propagation by disabling
///    the user actions and the stochastic processes during each step.
class Geant4eSession {
 public:
  static Geant4eSession& instance(
      G4VPhysicalVolume* world,
      const std::shared_ptr<const Acts::MagneticFieldProvider>& field) {
    // intentionally leaked: must outlive any static Geant4 teardown
    static Geant4eSession* session = new Geant4eSession(world, field);
    if ((world != nullptr && session->m_world != world) ||
        session->m_field != field.get()) {
      throw std::invalid_argument(
          "Geant4Stepper: the Geant4e session is already initialized with a "
          "different world volume or magnetic field");
    }
    return *session;
  }

  G4ErrorPropagatorManager* manager() { return m_manager; }

  bool attached() const { return m_attached; }

  /// Set the Geant4 step length limit (in CLHEP units)
  void setStepLengthLimit(double limit) {
    if (m_stepLimitProcess != nullptr) {
      m_stepLimitProcess->SetStepLimit(limit);
    } else {
      G4UImanager::GetUIpointer()->ApplyCommand(
          "/geant4e/limits/stepLength " + std::to_string(limit / CLHEP::mm) +
          " mm");
    }
  }

  /// Activate or deactivate the Geant4e mean energy loss process. In attach
  /// mode the activation is handled per step by the @c PropagationGuard.
  void setEnergyLossActive(bool active) {
    if (m_attached || active == m_energyLossActive) {
      return;
    }
    for (auto& [processManager, process] : m_energyLossProcesses) {
      processManager->SetProcessActivation(process, active);
    }
    m_energyLossActive = active;
  }

  /// Make sure the Geant4e limit and energy loss processes are attached to
  /// the given particle (attach mode only; they are added deactivated and
  /// only enabled inside the @c PropagationGuard)
  void prepareParticle(const G4ParticleDefinition& particle) {
    if (!m_attached) {
      return;
    }
    G4ProcessManager* processManager = particle.GetProcessManager();
    if (processManager == nullptr) {
      return;
    }
    G4ProcessVector* processes = processManager->GetProcessList();
    for (G4int i = 0; i < static_cast<G4int>(processes->size()); ++i) {
      if ((*processes)[i] == m_stepLimitProcess) {
        return;
      }
    }
    addProcessesTo(*processManager);
  }

  /// Scope guard for a single error propagation step in attach mode: forces
  /// the Geant4 state machine to GeomClosed, removes the simulation user
  /// actions, and disables all processes of the tracked particle except
  /// transportation and the Geant4e processes. Everything is restored on
  /// destruction and the Geant4e mode is reset to forward so that the
  /// simulation field equation is not affected.
  class PropagationGuard {
   public:
    PropagationGuard(Geant4eSession& session,
                     const G4ParticleDefinition& particle, bool energyLoss)
        : m_session(session) {
      m_stateManager = G4StateManager::GetStateManager();
      m_previousState = m_stateManager->GetCurrentState();
      if (m_previousState != G4State_GeomClosed) {
        m_stateManager->SetNewState(G4State_GeomClosed);
      }

      m_trackingManager =
          G4EventManager::GetEventManager()->GetTrackingManager();
      m_steppingManager = m_trackingManager->GetSteppingManager();
      m_trackingAction = m_trackingManager->GetUserTrackingAction();
      m_steppingAction = m_steppingManager->GetUserAction();
      m_trackingManager->SetUserAction(
          static_cast<G4UserTrackingAction*>(nullptr));
      m_steppingManager->SetUserAction(
          static_cast<G4UserSteppingAction*>(nullptr));

      m_processManager = particle.GetProcessManager();
      G4ProcessVector* processes = m_processManager->GetProcessList();
      m_activations.resize(processes->size());
      for (G4int i = 0; i < static_cast<G4int>(processes->size()); ++i) {
        m_activations[i] = m_processManager->GetProcessActivation(i);
        G4VProcess* process = (*processes)[i];
        const G4String& name = process->GetProcessName();
        const bool active =
            (name == "Transportation" || name == "CoupledTransportation" ||
             process == m_session.m_stepLimitProcess ||
             (process == m_session.m_energyLossProcess && energyLoss));
        m_processManager->SetProcessActivation(i, active);
      }
    }

    ~PropagationGuard() {
      for (G4int i = 0; i < static_cast<G4int>(m_activations.size()); ++i) {
        m_processManager->SetProcessActivation(i, m_activations[i]);
      }
      m_trackingManager->SetUserAction(m_trackingAction);
      m_steppingManager->SetUserAction(m_steppingAction);
      if (m_previousState != G4State_GeomClosed) {
        m_stateManager->SetNewState(m_previousState);
      }
      // leave the (global) propagation mode in forward, otherwise the
      // simulation would integrate with a flipped field sign
      G4ErrorPropagatorData::GetErrorPropagatorData()->SetMode(
          G4ErrorMode_PropForwards);
    }

    PropagationGuard(const PropagationGuard&) = delete;
    PropagationGuard& operator=(const PropagationGuard&) = delete;

   private:
    Geant4eSession& m_session;
    G4StateManager* m_stateManager = nullptr;
    G4ApplicationState m_previousState = G4State_PreInit;
    G4TrackingManager* m_trackingManager = nullptr;
    G4SteppingManager* m_steppingManager = nullptr;
    G4UserTrackingAction* m_trackingAction = nullptr;
    G4UserSteppingAction* m_steppingAction = nullptr;
    G4ProcessManager* m_processManager = nullptr;
    std::vector<G4bool> m_activations;
  };

 private:
  Geant4eSession(G4VPhysicalVolume* world,
                 const std::shared_ptr<const Acts::MagneticFieldProvider>&
                     field) {
    if (field == nullptr) {
      throw std::invalid_argument(
          "Geant4Stepper: a magnetic field is required");
    }
    m_field = field.get();

    m_attached = (G4RunManager::GetRunManager() != nullptr);

    if (m_attached) {
      initializeAttached(world);
    } else {
      initializeStandalone(world, field);
    }

    // G4ErrorPropagator dereferences the target at Geant4 volume boundaries,
    // so a (never reached) target is required for plain stepping
    m_target = std::make_unique<G4ErrorPlaneSurfaceTarget>(
        G4Normal3D(0., 0., 1.),
        G4Point3D(0., 0., std::numeric_limits<float>::max()));
    G4ErrorPropagatorData::GetErrorPropagatorData()->SetTarget(m_target.get());
  }

  /// Standalone setup: the session owns world, field and Geant4e physics
  void initializeStandalone(
      G4VPhysicalVolume* world,
      const std::shared_ptr<const Acts::MagneticFieldProvider>& field) {
    if (world == nullptr) {
      throw std::invalid_argument(
          "Geant4Stepper: a world volume is required (no initialized "
          "G4RunManager to attach to)");
    }
    m_world = world;

    m_manager = G4ErrorPropagatorManager::GetErrorPropagatorManager();
    m_manager->SetUserInitialization(world);

    // The field has to be installed before InitGeant4e, which hooks the
    // backward-mode equation of motion into the existing chord finder
    m_fieldAdapter = std::make_unique<Geant4FieldAdapter>(field);
    G4FieldManager* fieldManager =
        G4TransportationManager::GetTransportationManager()->GetFieldManager();
    fieldManager->SetDetectorField(m_fieldAdapter.get());
    fieldManager->CreateChordFinder(m_fieldAdapter.get());

    m_manager->InitGeant4e();

    collectProcesses();
  }

  /// Attach to an existing, initialized G4RunManager: share kernel, world,
  /// field and physics list. The G4ErrorPropagatorManager must have been
  /// created before the run manager initialized its physics (the Geant4e
  /// navigator replaces the tracking navigator that the transportation
  /// process caches at construction).
  void initializeAttached(G4VPhysicalVolume* world) {
    G4VPhysicalVolume* existingWorld =
        G4TransportationManager::GetTransportationManager()
            ->GetNavigatorForTracking()
            ->GetWorldVolume();
    if (existingWorld == nullptr) {
      throw std::invalid_argument(
          "Geant4Stepper: the G4RunManager geometry is not initialized yet");
    }
    if (world != nullptr && world != existingWorld) {
      throw std::invalid_argument(
          "Geant4Stepper: the configured world volume differs from the "
          "world of the existing G4RunManager");
    }
    m_world = existingWorld;

    m_manager = G4ErrorPropagatorManager::GetErrorPropagatorManager();
    // no SetUserInitialization: geometry, physics and field are taken from
    // the existing run manager; InitGeant4e only wraps the field equation
    // for backward mode and runs the kernel run initialization
    m_manager->InitGeant4e();

    // InitGeant4e only hooks the backward-mode equation of motion into the
    // *global* field manager; a field attached to the world logical volume
    // (as done by the ActsExamples Geant4 simulation) has to be wrapped here
    installBackwardsEquation(
        m_world->GetLogicalVolume()->GetFieldManager());

    // restore the Geant4 state machine for the next simulation run
    G4StateManager::GetStateManager()->SetNewState(G4State_Idle);
    G4ErrorPropagatorData::GetErrorPropagatorData()->SetMode(
        G4ErrorMode_PropForwards);

    // own Geant4e processes, attached lazily per particle (the simulation
    // physics list does not contain them)
    m_ownedStepLimitProcess =
        std::make_unique<G4ErrorStepLengthLimitProcess>();
    m_stepLimitProcess = m_ownedStepLimitProcess.get();
    m_ownedEnergyLossProcess = std::make_unique<G4ErrorEnergyLoss>();
    m_energyLossProcess = m_ownedEnergyLossProcess.get();
  }

  /// Replace the equation of motion of the given field manager's chord
  /// finder by the Geant4e one, which flips the field sign in backward
  /// propagation mode (mirrors G4ErrorPropagatorManager::
  /// InitFieldForBackwards for non-global field managers)
  static void installBackwardsEquation(G4FieldManager* fieldManager) {
    if (fieldManager == nullptr || fieldManager->GetChordFinder() == nullptr) {
      return;
    }
    auto* driver = fieldManager->GetChordFinder()->GetIntegrationDriver();
    if (driver == nullptr ||
        dynamic_cast<G4ErrorMag_UsualEqRhs*>(driver->GetEquationOfMotion()) !=
            nullptr) {
      return;
    }
    auto* field = static_cast<G4MagneticField*>(
        const_cast<G4Field*>(fieldManager->GetDetectorField()));
    auto* equation = new G4ErrorMag_UsualEqRhs(field);
    driver->SetEquationOfMotion(equation);
    auto* integrationStepper = new G4ClassicalRK4(equation);
    fieldManager->SetChordFinder(
        new G4ChordFinder(field, 1.0e-2 * CLHEP::mm, integrationStepper));
  }

  /// Add the Geant4e processes to a process manager (deactivated)
  void addProcessesTo(G4ProcessManager& processManager) {
    processManager.AddContinuousProcess(m_energyLossProcess, 1);
    processManager.AddDiscreteProcess(m_stepLimitProcess, 2);
    processManager.SetProcessActivation(m_energyLossProcess, false);
    processManager.SetProcessActivation(m_stepLimitProcess, false);
  }

  /// Locate the (shared) Geant4e limit and energy loss process instances
  /// registered by the G4ErrorPhysicsList (standalone mode)
  void collectProcesses() {
    G4ParticleTable* table = G4ParticleTable::GetParticleTable();
    for (G4int i = 0; i < table->entries(); ++i) {
      G4ParticleDefinition* particle = table->GetParticle(i);
      G4ProcessManager* processManager = particle->GetProcessManager();
      if (processManager == nullptr) {
        continue;
      }
      G4ProcessVector* processes = processManager->GetProcessList();
      for (G4int j = 0; j < static_cast<G4int>(processes->size()); ++j) {
        G4VProcess* process = (*processes)[j];
        if (process->GetProcessName() == "G4ErrorStepLengthLimit") {
          m_stepLimitProcess =
              static_cast<G4ErrorStepLengthLimitProcess*>(process);
        } else if (process->GetProcessName() == "G4ErrorEnergyLoss") {
          m_energyLossProcesses.emplace_back(processManager, process);
        }
      }
    }
  }

  G4VPhysicalVolume* m_world = nullptr;
  const Acts::MagneticFieldProvider* m_field = nullptr;
  G4ErrorPropagatorManager* m_manager = nullptr;
  bool m_attached = false;
  std::unique_ptr<Geant4FieldAdapter> m_fieldAdapter;
  std::unique_ptr<G4ErrorPlaneSurfaceTarget> m_target;
  G4ErrorStepLengthLimitProcess* m_stepLimitProcess = nullptr;
  G4VProcess* m_energyLossProcess = nullptr;
  std::unique_ptr<G4ErrorStepLengthLimitProcess> m_ownedStepLimitProcess;
  std::unique_ptr<G4ErrorEnergyLoss> m_ownedEnergyLossProcess;
  std::vector<std::pair<G4ProcessManager*, G4VProcess*>>
      m_energyLossProcesses;
  bool m_energyLossActive = true;
};

}  // namespace detail

Geant4Stepper::State::State(const Options& optionsIn,
                            Acts::MagneticFieldProvider::Cache fieldCacheIn)
    : options(optionsIn), fieldCache(std::move(fieldCacheIn)) {}

Geant4Stepper::State::State(State&&) noexcept = default;
Geant4Stepper::State& Geant4Stepper::State::operator=(State&&) noexcept =
    default;
Geant4Stepper::State::~State() = default;

Geant4Stepper::Geant4Stepper(Config config) : m_cfg(std::move(config)) {
  if (m_cfg.bField == nullptr) {
    throw std::invalid_argument("Geant4Stepper: a magnetic field is required");
  }
}

detail::Geant4eSession& Geant4Stepper::session() const {
  if (m_session == nullptr) {
    m_session =
        &detail::Geant4eSession::instance(m_cfg.worldVolume, m_cfg.bField);
  }
  return *m_session;
}

auto Geant4Stepper::makeState(const Options& options) const -> State {
  State state{options, m_cfg.bField->makeCache(options.magFieldContext)};
  return state;
}

void Geant4Stepper::initialize(State& state, const BoundParameters& par) const {
  initialize(state, par.parameters(), par.covariance(),
             par.particleHypothesis(), par.referenceSurface());
}

void Geant4Stepper::initialize(State& state,
                               const Acts::BoundVector& boundParams,
                               const std::optional<Acts::BoundMatrix>& cov,
                               Acts::ParticleHypothesis particleHypothesis,
                               const Acts::Surface& surface) const {
  // make sure the Geant4e session (and with it the particle table) exists
  session();

  const Acts::FreeVector freeParams = Acts::transformBoundToFreeParameters(
      surface, state.options.geoContext, boundParams);

  state.particleHypothesis = particleHypothesis;

  state.pathAccumulated = 0.;
  state.nSteps = 0;
  state.nStepTrials = 0;
  state.stepSize = Acts::ConstrainedStep();
  state.stepSize.setAccuracy(state.options.initialStepSize);
  state.stepSize.setUser(state.options.maxStepSize);
  state.previousStepSize = 0.;
  state.statistics = Acts::StepperStatistics();

  state.pars = freeParams;

  const double q = particleHypothesis.extractCharge(
      freeParams[Acts::eFreeQOverP]);
  const G4ParticleDefinition& particle =
      findG4Particle(particleHypothesis, q);
  state.g4Particle = &particle;

  state.covTransport = cov.has_value();
  state.jacobian = Jacobian::Identity();
  state.g4Transport = Acts::SquareMatrix<5>::Identity();

  G4ErrorTrajErr error(5, 0);
  if (state.covTransport) {
    state.cov = *cov;
    state.timeVariance = (*cov)(Acts::eBoundTime, Acts::eBoundTime);

    // rotate the bound covariance into the curvilinear frame at the start
    const Acts::Vector3 direction = freeParams.segment<3>(Acts::eFreeDir0);
    const Acts::BoundToFreeMatrix boundToFree = surface.boundToFreeJacobian(
        state.options.geoContext, freeParams.segment<3>(Acts::eFreePos0),
        direction);
    const auto fieldRes = getField(state, position(state));
    const Acts::Vector3 bField =
        fieldRes.ok() ? *fieldRes : Acts::Vector3::Zero();
    Acts::FreeToBoundMatrix freeToBound = Acts::FreeToBoundMatrix::Zero();
    Acts::BoundMatrix boundToCurvilinear = Acts::BoundMatrix::Zero();
    Acts::detail::boundToCurvilinearTransportJacobian(
        direction, boundToFree, Acts::FreeMatrix::Identity(), freeToBound,
        freeToPathDerivatives(freeParams, particleHypothesis, bField),
        boundToCurvilinear);
    const Acts::BoundMatrix curvilinearCov =
        boundToCurvilinear * (*cov) * boundToCurvilinear.transpose();

    error = curvilinearCovToG4e(curvilinearCov, q, false);
  }

  const Acts::Vector3 pos =
      position(state) / Acts::UnitConstants::mm * CLHEP::mm;
  const Acts::Vector3 mom =
      momentum(state) / Acts::UnitConstants::MeV * CLHEP::MeV;
  state.g4State = std::make_unique<G4ErrorFreeTrajState>(
      particle.GetParticleName(), G4Point3D(pos.x(), pos.y(), pos.z()),
      G4Vector3D(mom.x(), mom.y(), mom.z()), error);

  state.g4TrackNeedsInit = true;
  state.g4Mode = G4ErrorMode_PropForwards;
}

auto Geant4Stepper::boundState(
    State& state, const Acts::Surface& surface, bool transportCov,
    const Acts::FreeToBoundCorrection& /*freeToBoundCorrection*/) const
    -> Acts::Result<BoundState> {
  const auto boundParams = Acts::transformFreeToBoundParameters(
      state.pars, surface, state.options.geoContext);
  if (!boundParams.ok()) {
    return boundParams.error();
  }

  std::optional<Acts::BoundMatrix> cov = std::nullopt;
  if (state.covTransport && transportCov) {
    transportCovarianceToBound(state, surface);
    cov = state.cov;
  }

  return BoundState{
      BoundParameters(surface.getSharedPtr(), *boundParams, std::move(cov),
                      state.particleHypothesis),
      state.jacobian, state.pathAccumulated};
}

bool Geant4Stepper::prepareCurvilinearState(State& /*state*/) const {
  return true;
}

auto Geant4Stepper::curvilinearState(State& state, bool transportCov) const
    -> BoundState {
  std::optional<Acts::BoundMatrix> cov = std::nullopt;
  if (state.covTransport && transportCov) {
    transportCovarianceToCurvilinear(state);
    cov = state.cov;
  }

  const Acts::Vector4 pos4{state.pars[Acts::eFreePos0],
                           state.pars[Acts::eFreePos1],
                           state.pars[Acts::eFreePos2], time(state)};
  return BoundState{
      BoundParameters::createCurvilinear(pos4, direction(state),
                                         qOverP(state), std::move(cov),
                                         state.particleHypothesis),
      state.jacobian, state.pathAccumulated};
}

void Geant4Stepper::update(State& state, const Acts::FreeVector& freeParams,
                           const Acts::BoundVector& /*boundParams*/,
                           const Covariance& covariance,
                           const Acts::Surface& surface) const {
  state.pars = freeParams;
  state.cov = covariance;
  state.timeVariance = covariance(Acts::eBoundTime, Acts::eBoundTime);

  if (state.g4State != nullptr) {
    // re-express the error in the (physical direction) curvilinear frame
    const Acts::Vector3 direction = freeParams.segment<3>(Acts::eFreeDir0);
    const Acts::BoundToFreeMatrix boundToFree = surface.boundToFreeJacobian(
        state.options.geoContext, freeParams.segment<3>(Acts::eFreePos0),
        direction);
    const auto fieldRes = getField(state, position(state));
    const Acts::Vector3 bField =
        fieldRes.ok() ? *fieldRes : Acts::Vector3::Zero();
    Acts::FreeToBoundMatrix freeToBound = Acts::FreeToBoundMatrix::Zero();
    Acts::BoundMatrix boundToCurvilinear = Acts::BoundMatrix::Zero();
    Acts::detail::boundToCurvilinearTransportJacobian(
        direction, boundToFree, Acts::FreeMatrix::Identity(), freeToBound,
        freeToPathDerivatives(freeParams, state.particleHypothesis, bField),
        boundToCurvilinear);
    const Acts::BoundMatrix curvilinearCov =
        boundToCurvilinear * covariance * boundToCurvilinear.transpose();
    state.g4State->SetError(
        curvilinearCovToG4e(curvilinearCov, charge(state), false));
    state.g4Mode = G4ErrorMode_PropForwards;
    state.g4TrackNeedsInit = true;
  }
}

void Geant4Stepper::update(State& state, const Acts::Vector3& uposition,
                           const Acts::Vector3& udirection, double qOverP,
                           double time) const {
  state.pars.segment<3>(Acts::eFreePos0) = uposition;
  state.pars.segment<3>(Acts::eFreeDir0) = udirection;
  state.pars[Acts::eFreeQOverP] = qOverP;
  state.pars[Acts::eFreeTime] = time;
  state.g4TrackNeedsInit = true;
}

void Geant4Stepper::transportCovarianceToCurvilinear(State& state) const {
  if (!state.covTransport || state.g4State == nullptr) {
    return;
  }
  const bool reversed = (state.g4Mode == G4ErrorMode_PropBackwards);
  state.cov = g4eCovToCurvilinear(state.g4State->GetError(), charge(state),
                                  reversed, state.timeVariance);
  state.g4Transport = Acts::SquareMatrix<5>::Identity();
}

void Geant4Stepper::transportCovarianceToBound(
    State& state, const Acts::Surface& surface,
    const Acts::FreeToBoundCorrection& /*freeToBoundCorrection*/) const {
  if (!state.covTransport || state.g4State == nullptr) {
    return;
  }
  const bool reversed = (state.g4Mode == G4ErrorMode_PropBackwards);
  const Acts::BoundMatrix curvilinearCov = g4eCovToCurvilinear(
      state.g4State->GetError(), charge(state), reversed, state.timeVariance);

  // project the curvilinear covariance onto the target surface
  const auto fieldRes = getField(state, position(state));
  const Acts::Vector3 bField =
      fieldRes.ok() ? *fieldRes : Acts::Vector3::Zero();
  const Acts::BoundMatrix curvilinearToBound =
      Acts::detail::boundToBoundTransportJacobian(
          state.options.geoContext, state.pars,
          Acts::CurvilinearSurface(position(state), direction(state))
              .boundToFreeJacobian(),
          Acts::FreeMatrix::Identity(),
          freeToPathDerivatives(state.pars, state.particleHypothesis, bField),
          surface);

  state.cov = curvilinearToBound * curvilinearCov *
              curvilinearToBound.transpose();
  state.timeVariance = state.cov(Acts::eBoundTime, Acts::eBoundTime);
  state.g4Transport = Acts::SquareMatrix<5>::Identity();
}

Acts::Result<double> Geant4Stepper::step(
    State& state, Acts::Direction propDir,
    const Acts::IVolumeMaterial* /*material*/) const {
  detail::Geant4eSession& g4Session = session();

  if (state.g4State == nullptr || state.g4Particle == nullptr) {
    return Geant4StepperError::PropagationFailed;
  }

  // in attach mode shield the coexisting Geant4 simulation from the error
  // propagation for the duration of this step
  std::optional<detail::Geant4eSession::PropagationGuard> guard;
  if (g4Session.attached()) {
    g4Session.prepareParticle(*state.g4Particle);
    guard.emplace(g4Session, *state.g4Particle, m_cfg.meanEnergyLoss);
  }

  // signed step along the momentum direction
  const double signedH = state.stepSize.value() * propDir;
  const bool reverse = signedH < 0.;
  const G4ErrorMode mode =
      reverse ? G4ErrorMode_PropBackwards : G4ErrorMode_PropForwards;

  ++state.nStepTrials;
  ++state.statistics.nAttemptedSteps;

  if (state.g4TrackNeedsInit || mode != state.g4Mode) {
    // (re-)seed the Geant4 track from the ACTS-side parameters; in backward
    // mode the Geant4 track carries the reversed momentum and Geant4e
    // compensates with a flipped field sign and energy gain
    const Acts::Vector3 pos =
        position(state) / Acts::UnitConstants::mm * CLHEP::mm;
    Acts::Vector3 mom = momentum(state) / Acts::UnitConstants::MeV * CLHEP::MeV;
    if (reverse) {
      mom = -mom;
    }
    if (state.covTransport &&
        (mode == G4ErrorMode_PropBackwards) !=
            (state.g4Mode == G4ErrorMode_PropBackwards)) {
      state.g4State->SetError(flipG4eFrame(state.g4State->GetError()));
    }
    state.g4State->SetParameters(G4Point3D(pos.x(), pos.y(), pos.z()),
                                 G4Vector3D(mom.x(), mom.y(), mom.z()));
    g4Session.manager()->InitTrackPropagation();
    state.g4Mode = mode;
    state.g4TrackNeedsInit = false;
  }

  const double stepLimit =
      std::min(std::abs(signedH), m_cfg.maxStepLength);
  g4Session.setStepLengthLimit(stepLimit / Acts::UnitConstants::mm * CLHEP::mm);
  g4Session.setEnergyLossActive(m_cfg.meanEnergyLoss);

  std::optional<G4ErrorTrajErr> errorBefore;
  if (state.covTransport && !m_cfg.covarianceNoise) {
    errorBefore = state.g4State->GetError();
  }

  const G4int ierr =
      g4Session.manager()->PropagateOneStep(state.g4State.get(), mode);
  if (ierr != 0) {
    return Geant4StepperError::PropagationFailed;
  }

  const G4Track* track = state.g4State->GetG4Track();
  const double ds = track->GetStepLength() * kLengthG4ToActs;
  if (ds <= 0.) {
    if (track->GetNextVolume() == nullptr) {
      return Geant4StepperError::OutsideWorld;
    }
    if (track->GetTrackStatus() == fStopAndKill) {
      return Geant4StepperError::TrackKilled;
    }
  }
  if (track->GetNextVolume() == nullptr ||
      track->GetTrackStatus() == fStopAndKill) {
    // no further stepping possible with this Geant4 track
    state.g4TrackNeedsInit = true;
  }

  // harvest the step result, in backward mode the Geant4 momentum is the
  // reversed physical momentum
  const G4Point3D g4Pos = state.g4State->GetPosition();
  const G4Vector3D g4Mom = state.g4State->GetMomentum();
  const Acts::Vector3 newPosition =
      Acts::Vector3(g4Pos.x(), g4Pos.y(), g4Pos.z()) * kLengthG4ToActs;
  Acts::Vector3 newMomentum =
      Acts::Vector3(g4Mom.x(), g4Mom.y(), g4Mom.z()) * kEnergyG4ToActs;
  if (reverse) {
    newMomentum = -newMomentum;
  }

  const double q = charge(state);
  const double p = newMomentum.norm();
  const double h = reverse ? -ds : ds;

  state.pars.segment<3>(Acts::eFreePos0) = newPosition;
  state.pars.segment<3>(Acts::eFreeDir0) = newMomentum.normalized();
  state.pars[Acts::eFreeQOverP] = (q != 0.) ? q / p : 1. / p;
  state.pars[Acts::eFreeTime] +=
      h * std::hypot(1., state.particleHypothesis.mass() / p);

  if (state.covTransport) {
    const Acts::SquareMatrix<5> transfer =
        toEigen(state.g4State->GetTransfMat());
    state.g4Transport = transfer * state.g4Transport;
    if (!m_cfg.covarianceNoise) {
      // strip the multiple scattering and energy loss noise added by
      // Geant4e, keeping the pure parameter transport
      state.g4State->SetError(
          errorBefore->similarity(state.g4State->GetTransfMat()));
    }
  }

  state.pathAccumulated += h;
  ++state.nSteps;
  ++state.statistics.nSuccessfulSteps;
  if (propDir != Acts::Direction::fromScalarZeroAsPositive(signedH)) {
    ++state.statistics.nReverseSteps;
  }
  state.statistics.pathLength += h;
  state.statistics.absolutePathLength += ds;

  return h;
}

}  // namespace ActsPlugins

namespace {

class Geant4StepperErrorCategory : public std::error_category {
 public:
  // Return a short descriptive name for the category.
  const char* name() const noexcept final { return "Geant4StepperError"; }

  // Return what each enum means in text.
  std::string message(int c) const final {
    using ActsPlugins::Geant4StepperError;

    switch (static_cast<Geant4StepperError>(c)) {
      case Geant4StepperError::PropagationFailed:
        return "The Geant4 error propagation step failed";
      case Geant4StepperError::OutsideWorld:
        return "The Geant4 track left the Geant4 world volume";
      case Geant4StepperError::TrackKilled:
        return "The Geant4 track was stopped or killed during the step";
      default:
        return "unknown";
    }
  }
};

}  // namespace

std::error_code ActsPlugins::make_error_code(
    ActsPlugins::Geant4StepperError e) {
  static Geant4StepperErrorCategory c;
  return {static_cast<int>(e), c};
}
