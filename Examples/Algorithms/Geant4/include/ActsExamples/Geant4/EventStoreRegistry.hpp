// This file is part of the Acts project.
//
// Copyright (C) 2021 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "ActsExamples/EventData/SimHit.hpp"
#include "ActsExamples/EventData/SimParticle.hpp"

#include <vector>

namespace ActsExamples {

class WhiteBoard;

/// A regustry to event data and the event store per event
///
/// The access is static, however, there is an individual instance
/// per event and hence the retrival/writing is parallel event/save
///
/// @note multiple threads per event are not supported
class EventStoreRegistry {
 public:
  EventStoreRegistry(size_t nevents);
  virtual ~EventStoreRegistry() = default;

  static std::vector<WhiteBoard*> boards;
  static std::vector<SimHitContainer::sequence_type> hits;
  static std::vector<SimParticleContainer::sequence_type> particlesInitial;
  static std::vector<SimParticleContainer::sequence_type> particlesFinal;
};

}  // namespace ActsExamples
