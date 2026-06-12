// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Direction.hpp"
#include "Acts/Definitions/Units.hpp"
#include "Acts/EventData/BoundTrackParameters.hpp"
#include "Acts/EventData/detail/CorrectedTransformationFreeToBound.hpp"
#include "Acts/MagneticField/MagneticFieldProvider.hpp"
#include "Acts/Propagator/ConstrainedStep.hpp"
#include "Acts/Propagator/NavigationTarget.hpp"
#include "Acts/Propagator/PropagatorTraits.hpp"
#include "Acts/Propagator/StepperOptions.hpp"
#include "Acts/Propagator/StepperStatistics.hpp"
#include "Acts/Propagator/detail/SteppingHelper.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Utilities/Intersection.hpp"
#include "Acts/Utilities/Result.hpp"

#include <memory>
#include <optional>
#include <string>
#include <tuple>

class G4VPhysicalVolume;
class G4ErrorFreeTrajState;
class G4ParticleDefinition;

namespace Acts {
class IVolumeMaterial;
}  // namespace Acts

namespace ActsPlugins {

namespace detail {
class Geant4eSession;
}  // namespace detail

/// @brief Stepper based on the Geant4(e) error propagation machinery
///
/// This stepper fulfills @c Acts::StepperConcept and can be plugged into an
/// @c Acts::Propagator. Track parameter transport is performed by Geant4
/// (field integration, geometry traversal and - if configured - mean energy
/// loss), and covariance transport is performed by the Geant4e error
/// propagation module (`G4ErrorPropagator`), including multiple scattering
/// and energy loss noise contributions if configured.
///
/// At input and output the stepper speaks pure ACTS:
/// @c Acts::BoundTrackParameters on @c Acts::Surface objects. Surface
/// targeting is done by the ACTS navigator and the @c Acts::ConstrainedStep
/// machinery - Geant4 only honors the resulting per-step length limit, such
/// that the propagation result is expressed at the ACTS surface definition.
///
/// @note Limitations (inherent to Geant4's global state and Geant4e):
///  - Single-threaded use only; one Geant4 world per process; one active
///    propagation state at a time.
///  - The @c State is movable but not copyable.
///  - Coexistence with a standard `G4RunManager` (e.g. a Geant4 simulation
///    in the same job) is supported in an "attach" mode: the stepper then
///    shares the Geant4 kernel, world and field with the simulation. In
///    this mode the Geant4e machinery has to be prepared *before* the run
///    manager initializes its physics (the Geant4e navigator replaces the
///    tracking navigator), and during each step the user actions and all
///    stochastic physics processes of the tracked particle are temporarily
///    disabled.
///  - The Geant4 world volume must spatially enclose the ACTS geometry that
///    is being navigated.
///  - Geant4e does not transport time uncertainties: the time variance is
///    kept constant, time-parameter correlations are dropped.
///  - The bound-to-bound jacobian returned by @c boundState() /
///    @c curvilinearState() is the identity matrix: the covariance is
///    transported natively by Geant4e, but the transport jacobian is not
///    exposed in an ACTS-compatible way yet. Do not use this stepper for
///    Kalman smoothing.
class Geant4Stepper final {
 public:
  /// Type alias for bound track parameters
  using BoundParameters = Acts::BoundTrackParameters;
  /// Type alias for jacobian matrix
  using Jacobian = Acts::BoundMatrix;
  /// Type alias for covariance matrix
  using Covariance = Acts::BoundMatrix;
  /// Bound state tuple containing parameters, Jacobian, and path length
  using BoundState = std::tuple<BoundParameters, Jacobian, double>;

  /// Configuration for the Geant4 stepper.
  struct Config {
    /// The Geant4 world volume to propagate through. Geant4e is initialized
    /// lazily with this world on first use; all Geant4Stepper instances in a
    /// process must share the same world volume. May be `nullptr` if a
    /// `G4RunManager` with an initialized geometry exists in the process -
    /// the stepper then attaches to its world (attach mode).
    G4VPhysicalVolume* worldVolume = nullptr;

    /// Magnetic field provider (required). It is wrapped into a
    /// `G4MagneticField` and installed on the global Geant4 field manager.
    /// @note The field is always looked up with a default constructed
    ///       @c Acts::MagneticFieldContext inside Geant4.
    std::shared_ptr<const Acts::MagneticFieldProvider> bField;

    /// Apply mean energy loss to the track parameters while stepping
    /// (Geant4e `G4ErrorEnergyLoss` process)
    bool meanEnergyLoss = true;

    /// Include the Geant4e noise contributions (multiple scattering and
    /// energy loss fluctuations) in the covariance transport. If disabled,
    /// the covariance is transported with the pure parameter transport
    /// jacobian of each Geant4 step.
    bool covarianceNoise = true;

    /// Absolute cap of the Geant4 step length
    double maxStepLength = 1 * Acts::UnitConstants::m;
  };

  /// Stepper options including geometry and magnetic field contexts.
  struct Options : public Acts::StepperPlainOptions {
    /// Constructor from geometry and magnetic field contexts
    /// @param gctx The geometry context
    /// @param mctx The magnetic field context
    Options(const Acts::GeometryContext& gctx,
            const Acts::MagneticFieldContext& mctx)
        : StepperPlainOptions(gctx, mctx) {}

    /// Set plain options
    /// @param options The plain options to set
    void setPlainOptions(const Acts::StepperPlainOptions& options) {
      static_cast<Acts::StepperPlainOptions&>(*this) = options;
    }
  };

  /// @brief State for track parameter propagation
  ///
  /// The ACTS-side free parameter vector @c pars is the source of truth
  /// between Geant4 synchronization points; the owned
  /// `G4ErrorFreeTrajState` carries the Geant4e error matrix.
  struct State {
    /// Constructor from stepper options
    ///
    /// @param [in] optionsIn is the options object for the stepper
    /// @param [in] fieldCacheIn is the cache object for the magnetic field
    State(const Options& optionsIn,
          Acts::MagneticFieldProvider::Cache fieldCacheIn);

    State(const State&) = delete;
    State& operator=(const State&) = delete;
    /// Move constructor
    State(State&&) noexcept;
    /// Move assignment operator
    /// @return Reference to this state
    State& operator=(State&&) noexcept;
    ~State();

    /// Configuration options for the stepper
    Options options;

    /// Internal free vector parameters (ACTS units, including time)
    Acts::FreeVector pars = Acts::FreeVector::Zero();

    /// Particle hypothesis
    Acts::ParticleHypothesis particleHypothesis =
        Acts::ParticleHypothesis::pion();

    /// Covariance matrix (and indicator)
    /// associated with the initial error on track parameters
    bool covTransport = false;
    /// Covariance matrix at the last bound reference
    Covariance cov = Covariance::Zero();

    /// The full jacobian of the transport (identity, see class limitations)
    Jacobian jacobian = Jacobian::Identity();

    /// Accumulated Geant4e 5x5 transfer matrix since the last bound
    /// reference; used to strip noise when `Config::covarianceNoise` is off
    Acts::SquareMatrix<5> g4Transport = Acts::SquareMatrix<5>::Identity();

    /// Time variance of the start parameters (transported unchanged)
    double timeVariance = 0.;

    /// The Geant4e trajectory state, owns the Geant4e error matrix
    std::unique_ptr<G4ErrorFreeTrajState> g4State;

    /// The Geant4 particle definition matching the particle hypothesis
    const G4ParticleDefinition* g4Particle = nullptr;

    /// The Geant4 track needs to be (re-)seeded before the next step
    bool g4TrackNeedsInit = true;

    /// The last G4ErrorMode used for stepping (-1 if unset)
    int g4Mode = -1;

    /// Accumulated path length state
    double pathAccumulated = 0.;

    /// Total number of performed steps
    std::size_t nSteps = 0;

    /// Total number of attempted steps
    std::size_t nStepTrials = 0;

    /// Step size constrained by navigator, actors and user
    Acts::ConstrainedStep stepSize;

    /// Last performed step (for overstep limit calculation)
    double previousStepSize = 0.;

    /// Cache of the magnetic field provider for `getField` calls
    Acts::MagneticFieldProvider::Cache fieldCache;

    /// Statistics of the stepper
    Acts::StepperStatistics statistics;
  };

  /// @brief Constructor with configuration
  ///
  /// @param [in] config The configuration of the stepper
  explicit Geant4Stepper(Config config);

  /// Create a stepper state from given options
  /// @param options Configuration options for the stepper state
  /// @return Initialized stepper state object
  State makeState(const Options& options) const;

  /// Initialize the stepper state from bound track parameters
  /// @param state Stepper state to initialize
  /// @param par Bound track parameters to initialize from
  void initialize(State& state, const BoundParameters& par) const;

  /// Initialize the stepper state from bound parameters and surface
  /// @param state Stepper state to initialize
  /// @param boundParams Vector of bound track parameters
  /// @param cov Optional covariance matrix
  /// @param particleHypothesis Particle hypothesis for the track
  /// @param surface Surface associated with the parameters
  void initialize(State& state, const Acts::BoundVector& boundParams,
                  const std::optional<Acts::BoundMatrix>& cov,
                  Acts::ParticleHypothesis particleHypothesis,
                  const Acts::Surface& surface) const;

  /// Get the magnetic field at a given position
  ///
  /// @note This queries the ACTS magnetic field provider directly and does
  ///       not involve Geant4.
  ///
  /// @param [in,out] state is the stepper state, its field cache is used
  /// @param [in] pos is the field position
  /// @return Magnetic field vector at the given position or error
  Acts::Result<Acts::Vector3> getField(State& state,
                                       const Acts::Vector3& pos) const {
    return m_cfg.bField->getField(pos, state.fieldCache);
  }

  /// Global particle position accessor
  ///
  /// @param state [in] The stepping state
  /// @return Current global position vector
  Acts::Vector3 position(const State& state) const {
    return state.pars.template segment<3>(Acts::eFreePos0);
  }

  /// Momentum direction accessor
  ///
  /// @param state [in] The stepping state
  /// @return Current normalized direction vector
  Acts::Vector3 direction(const State& state) const {
    return state.pars.template segment<3>(Acts::eFreeDir0);
  }

  /// QoP accessor
  ///
  /// @param state [in] The stepping state
  /// @return Charge over momentum (q/p) value
  double qOverP(const State& state) const {
    return state.pars[Acts::eFreeQOverP];
  }

  /// Absolute momentum accessor
  ///
  /// @param state [in] The stepping state
  /// @return Absolute momentum magnitude
  double absoluteMomentum(const State& state) const {
    return particleHypothesis(state).extractMomentum(qOverP(state));
  }

  /// Momentum accessor
  ///
  /// @param state [in] The stepping state
  /// @return Current momentum vector
  Acts::Vector3 momentum(const State& state) const {
    return absoluteMomentum(state) * direction(state);
  }

  /// Charge access
  ///
  /// @param state [in] The stepping state
  /// @return Electric charge of the particle
  double charge(const State& state) const {
    return particleHypothesis(state).extractCharge(qOverP(state));
  }

  /// Particle hypothesis
  ///
  /// @param state [in] The stepping state
  /// @return Reference to the particle hypothesis used
  const Acts::ParticleHypothesis& particleHypothesis(
      const State& state) const {
    return state.particleHypothesis;
  }

  /// Time access
  ///
  /// @param state [in] The stepping state
  /// @return The time coordinate from the free parameters vector
  double time(const State& state) const { return state.pars[Acts::eFreeTime]; }

  /// Update surface status
  ///
  /// It checks the status to the reference surface & updates
  /// the step size accordingly
  ///
  /// @param [in,out] state The stepping state
  /// @param [in] surface The surface provided
  /// @param [in] index The surface intersection index
  /// @param [in] propDir The propagation direction
  /// @param [in] boundaryTolerance The boundary check for this status update
  /// @param [in] surfaceTolerance Surface tolerance used for intersection
  /// @param [in] stype The step size type to be set
  /// @param [in] logger A @c Logger instance
  /// @return Status of the intersection indicating whether surface was reached
  Acts::IntersectionStatus updateSurfaceStatus(
      State& state, const Acts::Surface& surface, std::uint8_t index,
      Acts::Direction propDir,
      const Acts::BoundaryTolerance& boundaryTolerance,
      double surfaceTolerance, Acts::ConstrainedStep::Type stype,
      const Acts::Logger& logger = Acts::getDummyLogger()) const {
    return Acts::detail::updateSingleSurfaceStatus<Geant4Stepper>(
        *this, state, surface, index, propDir, boundaryTolerance,
        surfaceTolerance, stype, logger);
  }

  /// Update step size from a navigation target
  ///
  /// @param state [in,out] The stepping state
  /// @param target [in] The NavigationTarget
  /// @param direction [in] The propagation direction
  /// @param stype [in] The step size type to be set
  void updateStepSize(State& state, const Acts::NavigationTarget& target,
                      Acts::Direction direction,
                      Acts::ConstrainedStep::Type stype) const {
    static_cast<void>(direction);
    double stepSize = target.pathLength();
    updateStepSize(state, stepSize, stype);
  }

  /// Update step size - explicitly with a double
  ///
  /// @param state [in,out] The stepping state
  /// @param stepSize [in] The step size value
  /// @param stype [in] The step size type to be set
  void updateStepSize(State& state, double stepSize,
                      Acts::ConstrainedStep::Type stype) const {
    state.previousStepSize = state.stepSize.value();
    state.stepSize.update(stepSize, stype);
  }

  /// Get the step size
  ///
  /// @param state [in] The stepping state
  /// @param stype [in] The step size type to be returned
  /// @return Current step size for the specified constraint type
  double getStepSize(const State& state,
                     Acts::ConstrainedStep::Type stype) const {
    return state.stepSize.value(stype);
  }

  /// Release the step size
  ///
  /// @param state [in,out] The stepping state
  /// @param [in] stype The step size type to be released
  void releaseStepSize(State& state, Acts::ConstrainedStep::Type stype) const {
    state.stepSize.release(stype);
  }

  /// Output the step size - single component
  ///
  /// @param state [in,out] The stepping state
  /// @return String representation of the current step size
  std::string outputStepSize(const State& state) const {
    return state.stepSize.toString();
  }

  /// Create and return the bound state at the current position
  ///
  /// @brief This transports (if necessary) the covariance
  /// to the surface and creates a bound state. It does not check
  /// if the transported state is at the surface, this needs to
  /// be guaranteed by the propagator
  ///
  /// @param [in] state State that will be presented as @c BoundState
  /// @param [in] surface The surface to which we bind the state
  /// @param [in] transportCov Flag steering covariance transport
  /// @param [in] freeToBoundCorrection Correction for non-linearity effect during transform from free to bound
  ///
  /// @return A bound state:
  ///   - the parameters at the surface
  ///   - the stepwise jacobian towards it (identity, see class limitations)
  ///   - and the path length (from start - for ordering)
  Acts::Result<BoundState> boundState(
      State& state, const Acts::Surface& surface, bool transportCov = true,
      const Acts::FreeToBoundCorrection& freeToBoundCorrection =
          Acts::FreeToBoundCorrection(false)) const;

  /// @brief If necessary fill additional members needed for curvilinearState
  ///
  /// @param [in, out] state The state of the stepper
  /// @return true if nothing is missing after this call, false otherwise.
  bool prepareCurvilinearState(State& state) const;

  /// Create and return a curvilinear state at the current position
  ///
  /// @brief This transports (if necessary) the covariance
  /// to the current position and creates a curvilinear state.
  ///
  /// @param [in] state State that will be presented as @c CurvilinearState
  /// @param [in] transportCov Flag steering covariance transport
  ///
  /// @return A curvilinear state:
  ///   - the curvilinear parameters at given position
  ///   - the stepwise jacobian towards it (identity, see class limitations)
  ///   - and the path length (from start - for ordering)
  BoundState curvilinearState(State& state, bool transportCov = true) const;

  /// Method to update a stepper state to the some parameters
  ///
  /// @param [in,out] state State object that will be updated
  /// @param [in] freeParams Free parameters that will be written into @p state
  /// @param [in] boundParams Corresponding bound parameters
  /// @param [in] covariance The covariance that will be written into @p state
  /// @param [in] surface The surface the bound parameters are defined on
  void update(State& state, const Acts::FreeVector& freeParams,
              const Acts::BoundVector& boundParams,
              const Covariance& covariance, const Acts::Surface& surface) const;

  /// Method to update the stepper state
  ///
  /// @param [in,out] state State object that will be updated
  /// @param [in] uposition the updated position
  /// @param [in] udirection the updated direction
  /// @param [in] qOverP the updated qOverP value
  /// @param [in] time the updated time value
  void update(State& state, const Acts::Vector3& uposition,
              const Acts::Vector3& udirection, double qOverP,
              double time) const;

  /// Method for on-demand transport of the covariance
  /// to a new curvilinear frame at current position,
  /// or direction of the state
  ///
  /// @param [in,out] state State of the stepper
  void transportCovarianceToCurvilinear(State& state) const;

  /// Method for on-demand transport of the covariance to a surface
  ///
  /// @param [in,out] state State of the stepper
  /// @param [in] surface is the surface to which the covariance is forwarded to
  /// @param [in] freeToBoundCorrection Correction for non-linearity effect during transform from free to bound
  /// @note no check is done if the position is actually on the surface
  void transportCovarianceToBound(
      State& state, const Acts::Surface& surface,
      const Acts::FreeToBoundCorrection& freeToBoundCorrection =
          Acts::FreeToBoundCorrection(false)) const;

  /// Perform a Geant4(e) track parameter propagation step
  ///
  /// @param [in,out] state State of the stepper
  /// @param propDir is the direction of propagation
  /// @param material is the optional volume material we are stepping through.
  ///        This is ignored - the material is taken from the Geant4 geometry.
  /// @return the signed step length of the performed step
  ///
  /// @note The state contains the desired step size. Geant4 may perform a
  ///       shorter step, e.g. at volume boundaries of the Geant4 geometry.
  Acts::Result<double> step(State& state, Acts::Direction propDir,
                            const Acts::IVolumeMaterial* material) const;

 private:
  /// Lazily initialized Geant4e session (process-wide)
  /// @return Reference to the session
  detail::Geant4eSession& session() const;

  /// Configuration of the stepper
  Config m_cfg;

  /// Cached pointer to the process-wide Geant4e session
  mutable detail::Geant4eSession* m_session = nullptr;
};

}  // namespace ActsPlugins

namespace Acts {

template <>
struct SupportsBoundParameters<ActsPlugins::Geant4Stepper>
    : public std::true_type {};

}  // namespace Acts
