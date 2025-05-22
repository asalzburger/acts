// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/DD4hepDetector/DD4hepAlignmentDecorator.hpp"
#include "Acts/Plugins/DD4hep/DD4hepGeometryContext.hpp"
#include <limits>

ActsExamples::DD4hepAlignmentDecorator::DD4hepAlignmentDecorator(
    const Config& cfg, std::unique_ptr<const Acts::Logger> logger)
    : m_cfg(cfg), m_logger(std::move(logger)) {
  if (m_cfg.transformStores.empty() && m_cfg.nominalStore == nullptr) {
    throw std::invalid_argument(
        "Missing alignment stores (and nominal store), run without alignment "
        "decorator!");
  }
  // Sort on leading IOV
  std::sort(m_cfg.transformStores.begin(), m_cfg.transformStores.end(),
            [](const auto& lhs, const auto& rhs) {
              const auto& [lhsIov, lhsStore] = lhs;
              const auto& [rhsIov, rhsStore] = rhs;
              return lhsIov[0u] < rhsIov[0u];
            });
  // Check for overlapping IOVs
  for (const auto [istore, iovStore] : Acts::enumerate(m_cfg.transformStores)) {
    if (istore > 0) {
      const auto& [iov, store] = iovStore;
      const auto& [prevIov, prevStore] = m_cfg.transformStores[istore - 1];
      if (iov[0] == prevIov[0] || prevIov[1] >= iov[0]) {
        throw std::invalid_argument(
            "Intersecting IOVs found as [" + std::to_string(prevIov[0]) + ", " +
            std::to_string(prevIov[1]) + "] and [" + std::to_string(iov[0]) +
            ", " + std::to_string(iov[1]) + "]");
      }
    }
  }
}

ActsExamples::ProcessCode ActsExamples::DD4hepAlignmentDecorator::decorate(
    AlgorithmContext& context) {
  // Retrieve the event number from the context
  std::size_t eventNumber = context.eventNumber;

  // Check if an AlignmentStore struct already exists for this context
  // all we need is a pointer to it, in order to connect the delegate
  const AlignmentStore* alignmentStore = nullptr;
  auto foundAlignmentStore =
      std::find_if(m_alignmentStores.begin(), m_alignmentStores.end(),
                   [eventNumber](const auto& aStore) {
                     const auto& [iov, store] = aStore;
                     return iov[0] >= eventNumber && eventNumber <= iov[1];
                   });
  if (foundAlignmentStore != m_alignmentStores.end()) {
    // Found an existing alignment store, take it
    const auto& [iov, aStore] = *foundAlignmentStore;
    alignmentStore = &aStore;
    ACTS_VERBOSE("Found alignment store for event number " +
                 std::to_string(eventNumber) + " in [" +
                 std::to_string(iov[0]) + ", " + std::to_string(iov[1]) + "]");
  } else {
    // Start with the current alignment store
    auto currentStore = m_cfg.nominalStore;
    std::array<std::size_t, 2u> currentIov = {0, std::numeric_limits<std::size_t>::max()};
    auto matchedStore =
        std::find_if(m_cfg.transformStores.begin(), m_cfg.transformStores.end(),
                     [eventNumber](const auto& iovStore) {
                       const auto& [iov, store] = iovStore;
                       return iov[0] >= eventNumber && eventNumber <= iov[1];
                     });
    if (matchedStore != m_cfg.transformStores.end()) {
      const auto& [iov, store] = *matchedStore;
      currentIov = iov;
      ACTS_VERBOSE("Found transform store for event number " +
                   std::to_string(eventNumber) + " in [" +
                   std::to_string(iov[0]) + ", " + std::to_string(iov[1]) +
                   "]");
      ACTS_VERBOSE("Creating a new AlignmentStore from it");
      currentStore = store;
    }

    // We must have a valid alignment store at this point
    if (currentStore == nullptr) {
      throw std::invalid_argument(
          "No alignment store found for event number " +
          std::to_string(eventNumber) +
          ", check IOV bounds and/or configuration of nominal alignment store");
    }
    // Set the alignment store in the context
    m_alignmentStores.emplace_back(currentIov, AlignmentStore(currentStore.get()));
    auto [_, foundStore] = m_alignmentStores.back();
    alignmentStore = &foundStore;
  }

  // Create a DetectorElement alignment store for this context
  Acts::DD4hepGeometryContext::Alignment currentAlignment;
  currentAlignment.connect<&AlignmentStore::transform>(alignmentStore);
  // Now decorate the context with it
  context.geoContext = Acts::DD4hepGeometryContext(currentAlignment);
  return ActsExamples::ProcessCode::SUCCESS;
}
