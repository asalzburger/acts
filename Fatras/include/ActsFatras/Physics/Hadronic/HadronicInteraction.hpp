// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "ActsFatras/EventData/Particle.hpp"

#include <vector>

namespace ActsFatras {

/// This class handles the hadronic interaction. It evaluates the distance
/// after which the interaction will occur and the final state due the
/// interaction itself.
class HadronicInteraction {
 public:
  /// Method for evaluating the distance after which the hadronic
  /// will occur.
  ///
  /// @tparam generator_t Type of the random number generator
  /// @param [in, out] generator The random number generator
  /// @param [in] particle The particle
  ///
  /// @return valid X0 limit and no limit on L0
  template <typename generator_t>
  std::pair<double, double> generatePathLimits(generator_t& generator,
                                               const Particle& particle) const;

  /// This method evaluates the final state due to the photon conversion.
  ///
  /// @tparam generator_t Type of the random number generator
  /// @param [in, out] generator The random number generator
  /// @param [in, out] particle The interacting photon
  /// @param [out] generated List of generated particles
  ///
  /// @return True if the conversion occurred, else false
  template <typename generator_t>
  bool run(generator_t& generator, Particle& particle,
           std::vector<Particle>& generated) const;
};

template <typename generator_t>
std::pair<double, double> HadronicInteraction::generatePathLimits(
    generator_t& generator, const Particle& /**particle*/) const {
  std::uniform_real_distribution<double> uniformDistribution{0., 1.};

  const double u = uniformDistribution(generator);
  double freePathL0 = -std::log(u);  // in units of lambda_I
  return {std::numeric_limits<double>::infinity(), freePathL0};
}

template <typename generator_t>
bool HadronicInteraction::run(generator_t& /**generator*/, Particle& particle,
                              std::vector<Particle>& /**generated*/) const {
  // set the partice to be killed
  particle.setOutcome(ParticleOutcome::KilledInteraction);
  return true;
}

}  // namespace ActsFatras