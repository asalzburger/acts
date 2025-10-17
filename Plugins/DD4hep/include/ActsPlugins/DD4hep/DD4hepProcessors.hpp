// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <any>
#include <tuple>

#include <DD4hep/DetElement.h>

namespace ActsPlugins {

/// Visitor to count sensitive volumes in a DD4hep DetElement tree
struct DD4hepSensitiveCounter {
  struct Cache {
    std::size_t nSensitiveVolumes = 0;
  };

  /// @brief Generate a cache object
  Cache generateCache() const { return Cache(); }

  /// @brief Print the name of the DetElement
  /// @param detElement The DetElement to print
  void operator()(const dd4hep::DetElement& detElement, Cache& cache) {
    if (detElement.volume().isSensitive()) {
      cache.nSensitiveVolumes++;
    }
  }
};

struct DD4hepGraphVizPrinter {
  struct Cache {
    /// Restrict to level: -1 is all
    int maxLevel = 1;
    /// Current level
    std::size_t currentLevel = 0;
    /// Stream to store the graphviz output
    std::stringstream stream;
  };

  /// @brief Generate a cache object
  Cache generateCache() const { return Cache(); }

  /// @brief Print the name of the DetElement
  /// @param detElement The DetElement to print
  void operator()(const dd4hep::DetElement& detElement, Cache& cache);

  /// Chained visitor for the Detector element traversing
  template <typename... processors_t>
  struct DD4hepChainedProcessor {
    struct Cache {
      /// The caches
      std::array<std::any, sizeof...(processors_t)> subCaches;
    };

    /// Generate the chained cache object
    Cache generateCache() const {
      // Unfold the tuple and add the attachers
      Cache cache;
      std::size_t it = 0;
      std::apply(
          [&](auto&&... processor) {
            ((cache.subCaches[it++] = processor.generateCache()), ...);
          },
          processors);
      return cache;
    }

    /// The stored processors
    std::tuple<processors_t...> processors;

    /// Constructor
    DD4hepChainedProcessor(std::tuple<processors_t...> procs)
        : processors(std::move(procs)) {}

    /// @brief Print the name of the DetElement
    /// @param detElement The DetElement to print
    void operator()(const dd4hep::DetElement& detElement, Cache& cache) {
      std::size_t it = 0;
      std::apply(
          [&](auto&&... processor) {
            (processor(detElement, std::any_cast<decltype(processor)::Cache>(
                                       cache.subCaches[it++])),
             ...);
          },
          processors);
    }
  };

}  // namespace ActsPlugins
