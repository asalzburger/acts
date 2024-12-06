// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/Io/Common/SimHitHelper.hpp"

std::unordered_map<std::size_t, std::vector<Acts::Vector4>>
ActsExamples::SimHitHelper::associateHitsToParticle(
    const ActsExamples::SimHitContainer& simHits,
    double simParticleThreshold,
    const SimParticleContainer& simParticles) {
  // We need to associate first
  std::unordered_map<std::size_t, std::vector<Acts::Vector4>> particleHits;
  // Pre-loop over hits ... write those below threshold
  for (const auto& simHit : simHits) {
    double momentum = simHit.momentum4Before().head<3>().norm();
    if (momentum < simParticleThreshold) {
      continue;
    }
    if (particleHits.find(simHit.particleId().value()) == particleHits.end()) {
      particleHits[simHit.particleId().value()] = {};
    }
    particleHits[simHit.particleId().value()].push_back(simHit.fourPosition());
  }
  // Add the vertex if you have it
  for (auto& [pID, pHits] : particleHits) {
    if (!pHits.empty() && simParticles.contains(pID)) {
      const auto& simParticle = simParticles.find(pID);
      const auto& vertex = simParticle->initial().fourPosition();
      pHits.push_back(vertex);
    }
    // Sort along time
    std::sort(pHits.begin(), pHits.end(),
              [](const Acts::Vector4& a, const Acts::Vector4& b) {
                return a[Acts::eTime] < b[Acts::eTime];
              });
  }

  return particleHits;
}
