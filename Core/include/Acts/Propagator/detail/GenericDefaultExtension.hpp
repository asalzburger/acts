// This file is part of the Acts project.
//
// Copyright (C) 2018-2019 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Utilities/Helpers.hpp"

#include <array>

namespace Acts {

namespace {

static FreeMatrix freeMatrixZero = FreeMatrix::Zero();
static FreeMatrix freeMatrixIdentity = FreeMatrix::Identity();

static ActsMatrixD<3, 3> threeMatrixZero = ActsMatrixD<3, 3>::Zero();
static ActsMatrixD<3, 3> threeMatrixIdentity = ActsMatrixD<3, 3>::Identity();

static Vector3D threeVectorZero = Vector3D::Zero();

}  // namespace

namespace detail {

/// @brief Default evaluater of the k_i's and elements of the transport matrix
/// D of the RKN4 stepping. This is a pure implementation by textbook.
/// @note This it templated on the scalar type because of the autodiff plugin.
template <typename scalar_t>
struct GenericDefaultExtension {
  using Scalar = scalar_t;
  /// @brief Vector3D replacement for the custom scalar type
  using ThisVector3 = Acts::ActsVector<Scalar, 3>;

  /// @brief Control function if the step evaluation would be valid
  ///
  /// @tparam propagator_state_t Type of the state of the propagator
  /// @tparam stepper_t Type of the stepper
  /// @return Boolean flag if the step would be valid
  template <typename propagator_state_t, typename stepper_t>
  int bid(const propagator_state_t& /*unused*/,
          const stepper_t& /*unused*/) const {
    return 1;
  }

  /// @brief Evaluater of the k_i's of the RKN4. For the case of i = 0 this
  /// step sets up qop, too.
  ///
  /// @tparam propagator_state_t Type of the state of the propagator
  /// @tparam stepper_t Type of the stepper
  /// @param [in] state State of the propagator
  /// @param [in] stepper Stepper of the propagation
  /// @param [out] knew Next k_i that is evaluated
  /// @param [in] bField B-Field at the evaluation position
  /// @param [out] kQoP k_i elements of the momenta
  /// @param [in] i Index of the k_i, i = [0, 3]
  /// @param [in] h Step size (= 0. ^ 0.5 * StepSize ^ StepSize)
  /// @param [in] kprev Evaluated k_{i - 1}
  /// @return Boolean flag if the calculation is valid
  template <typename propagator_state_t, typename stepper_t>
  bool k(const propagator_state_t& state, const stepper_t& stepper,
         ThisVector3& knew, const Vector3D& bField, std::array<Scalar, 4>& kQoP,
         const int i = 0, const double h = 0.,
         const ThisVector3& kprev = ThisVector3()) {
    auto qop =
        stepper.charge(state.stepping) / stepper.momentum(state.stepping);
    // First step does not rely on previous data
    if (i == 0) {
      knew = qop * stepper.direction(state.stepping).cross(bField);
      kQoP = {0., 0., 0., 0.};
    } else {
      knew =
          qop * (stepper.direction(state.stepping) + h * kprev).cross(bField);
    }
    return true;
  }

  /// @brief Veto function after a RKN4 step was accepted by judging on the
  /// error of the step. Since the textbook does not deliver further vetos,
  /// this is a dummy function.
  ///
  /// @tparam propagator_state_t Type of the state of the propagator
  /// @tparam stepper_t Type of the stepper
  /// @param [in] state State of the propagator
  /// @param [in] stepper Stepper of the propagation
  /// @param [in] h Step size
  /// @return Boolean flag if the calculation is valid
  template <typename propagator_state_t, typename stepper_t>
  bool finalize(propagator_state_t& state, const stepper_t& stepper,
                const double h) const {
    propagateTime(state, stepper, h);
    return true;
  }

  /// @brief Veto function after a RKN4 step was accepted by judging on the
  /// error of the step. Since the textbook does not deliver further vetos,
  /// this is just for the evaluation of the transport matrix.
  ///
  /// @tparam propagator_state_t Type of the state of the propagator
  /// @tparam stepper_t Type of the stepper
  /// @param [in] state State of the propagator
  /// @param [in] stepper Stepper of the propagation
  /// @param [in] h Step size
  /// @param [out] D Transport matrix
  /// @return Boolean flag if the calculation is valid
  template <typename propagator_state_t, typename stepper_t>
  bool finalize(propagator_state_t& state, const stepper_t& stepper,
                const double h, FreeMatrix& D) const {
    return transportMatrix(state, stepper, h, propagateTime(state, stepper, h),
                           D);
  }

 private:
  /// @brief Propagation function for the time coordinate
  ///
  /// @tparam propagator_state_t Type of the state of the propagator
  /// @tparam stepper_t Type of the stepper
  /// @param [in, out] state State of the propagator
  /// @param [in] stepper Stepper of the propagation
  /// @param [in] h Step size
  template <typename propagator_state_t, typename stepper_t>
  double propagateTime(propagator_state_t& state, const stepper_t& stepper,
                       const double h) const {
    /// This evaluation is based on dt/ds = 1/v = 1/(beta * c) with the velocity
    /// v, the speed of light c and beta = v/c. This can be re-written as dt/ds
    /// = sqrt(m^2/p^2 + c^{-2}) with the mass m and the momentum p.
    auto derivative =
        std::hypot(1, state.options.mass / stepper.momentum(state.stepping));
    state.stepping.pars[eFreeTime] += h * derivative;
    if (state.stepping.covTransport) {
      state.stepping.derivative(3) = derivative;
    }
    return derivative;
  }

  /// @brief Calculates the transport matrix D for the jacobian
  ///
  /// @tparam propagator_state_t Type of the state of the propagator
  /// @tparam stepper_t Type of the stepper
  /// @param [in] state State of the propagator
  /// @param [in] stepper Stepper of the propagation
  /// @param [in] h Step size
  /// @param [in] pre-calculated dTdS
  /// @param [out] D Transport matrix
  /// @return Boolean flag if evaluation is valid
  template <typename propagator_state_t, typename stepper_t>
  bool transportMatrix(propagator_state_t& state, const stepper_t& stepper,
                       const double h, const double dTdS, FreeMatrix& D) const {
    /// The calculations are based on ATL-SOFT-PUB-2009-002. The update of the
    /// Jacobian matrix is requires only the calculation of eq. 17 and 18.
    /// Since the terms of eq. 18 are currently 0, this matrix is not needed
    /// in the calculation. The matrix A from eq. 17 consists out of 3
    /// different parts. The first one is given by the upper left 3x3 matrix
    /// that are calculated by the derivatives dF/dT (called dFdT) and dG/dT
    /// (calles dGdT). The second is given by the top 3 lines of the rightmost
    /// column. This is calculated by dFdL and dGdL. The remaining non-zero term
    /// is calculated directly. The naming of the variables is explained in eq.
    /// 11 and are directly related to the initial problem in eq. 7.
    /// The evaluation is based by propagating the parameters T and lambda as
    /// given in eq. 16 and evaluating the derivations for matrix A.
    /// @note The translation for u_{n+1} in eq. 7 is in this case a
    /// 3-dimensional vector without a dependency of Lambda or lambda neither in
    /// u_n nor in u_n'. The second and fourth eq. in eq. 14 have the constant
    /// offset matrices h * Id and Id respectively. This involves that the
    /// constant offset does not exist for rectangular matrix dGdu' (due to the
    /// missing Lambda part) and only exists for dFdu' in dlambda/dlambda.

    auto& sd = state.stepping.stepData;
    auto dir = stepper.direction(state.stepping);
    auto qop =
        stepper.charge(state.stepping) / stepper.momentum(state.stepping);

    D = freeMatrixZero;

    double half_h = h * 0.5;
    // This sets the reference to the sub matrices
    // dFdx is already initialised as (3x3) idendity
    auto dFdT = D.block<3, 3>(0, 4);
    auto dFdL = D.block<3, 1>(0, 7);
    // dGdx is already initialised as (3x3) zero
    auto dGdT = D.block<3, 3>(4, 4);
    auto dGdL = D.block<3, 1>(4, 7);

    ActsMatrixD<3, 3> dk1dT = threeMatrixZero;
    ActsMatrixD<3, 3> dk2dT = threeMatrixIdentity;
    ActsMatrixD<3, 3> dk3dT = threeMatrixIdentity;
    ActsMatrixD<3, 3> dk4dT = threeMatrixIdentity;

    // For the case without energy loss
    Vector3D dk1dL = dir.cross(sd.B_first);
    Vector3D dk2dL =
        (dir + half_h * sd.k1 + qop * half_h * dk1dL).cross(sd.B_middle);
    Vector3D dk3dL =
        (dir + half_h * sd.k2 + qop * half_h * dk2dL).cross(sd.B_middle);
    Vector3D dk4dL = (dir + h * sd.k3 + qop * h * dk3dL).cross(sd.B_last);

    dk1dT(0, 1) = qop * sd.B_first.z();
    dk1dT(0, 2) = qop * (-sd.B_first.y());
    dk1dT(1, 0) = qop * (-sd.B_first.z());
    dk1dT(1, 2) = qop * sd.B_first.x();
    dk1dT(2, 0) = qop * sd.B_first.y();
    dk1dT(2, 1) = qop * (-sd.B_first.x());

    dk2dT += half_h * dk1dT;
    dk2dT = qop * dk2dT.colwise().cross(sd.B_middle);

    dk3dT += half_h * dk2dT;
    dk3dT = qop * dk3dT.colwise().cross(sd.B_middle);

    dk4dT += h * dk3dT;
    dk4dT = qop * dk4dT.colwise().cross(sd.B_last);

    dFdT.setIdentity();
    dFdT += h / 6. * (dk1dT + dk2dT + dk3dT);
    dFdT *= h;

    dFdL = (h * h) / 6. * (dk1dL + dk2dL + dk3dL);

    dGdT += h / 6. * (dk1dT + 2. * (dk2dT + dk3dT) + dk4dT);

    dGdL = h / 6. * (dk1dL + 2. * (dk2dL + dk3dL) + dk4dL);

    D(3, 7) = h * state.options.mass * state.options.mass *
              stepper.charge(state.stepping) /
              (stepper.momentum(state.stepping) * dTdS);
    return true;
  }
};

}  // namespace detail

}  // namespace Acts
