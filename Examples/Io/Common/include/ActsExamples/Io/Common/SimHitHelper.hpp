// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "ActsExamples/EventData/SimHit.hpp"
#include "ActsExamples/EventData/SimParticle.hpp"

#include <unordered_map>

namespace ActsExamples::SimHitHelper {

/// @brief Associate hits to a particle
///
/// @param simHits is the container of sim hits
/// @param simParticleThreshold is the threshold to associate the hits
/// @param simParticles is the container of sim particles
///
/// @return a map of particle id to vector of hits
std::unordered_map<std::size_t, std::vector<Acts::Vector4>>
associateHitsToParticle(const SimHitContainer& simHits,
                        double simParticleThreshold = 0.,
                        const SimParticleContainer& simParticles = {});

}  // namespace ActsExamples::SimHitHelper
