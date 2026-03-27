// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsFatras/Physics/StandardInteractions.hpp"

ActsFatras::StandardChargedInteractions
ActsFatras::makeStandardChargedInteractions(
    double minimumAbsMomentum) {
  StandardChargedInteractions pl;
  pl.get<detail::StandardBetheBloch>().selectOutputParticle.valMin =
      minimumAbsMomentum;
  pl.get<detail::StandardBetheHeitler>().selectOutputParticle.valMin =
      minimumAbsMomentum;
  pl.get<detail::StandardBetheHeitler>().selectChildParticle.valMin =
      minimumAbsMomentum;
  return pl;
}

ActsFatras::StandardNeutralInteractions
ActsFatras::makeStandardNeutralInteractions(
    double minimumAbsMomentum) {
  StandardNeutralInteractions pl;
  pl.get<detail::StandardPhotonConversion>().selectOutputParticle.valMin =
      minimumAbsMomentum;
  pl.get<detail::StandardPhotonConversion>().selectChildParticle.valMin =
      minimumAbsMomentum;
  return pl;
}
