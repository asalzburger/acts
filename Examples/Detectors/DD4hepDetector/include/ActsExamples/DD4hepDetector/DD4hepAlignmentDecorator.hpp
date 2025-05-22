// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Geometry/TransformStore.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/Framework/AlgorithmContext.hpp"
#include "ActsExamples/Framework/IContextDecorator.hpp"
#include "ActsExamples/Framework/ProcessCode.hpp"
#include "Acts/Plugins/DD4hep/DD4hepDetectorElement.hpp"

#include <array>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

namespace ActsExamples {
struct AlgorithmContext;

/// @brief A simple alignment decorator for the DD4hep geometry
/// allowing to load a single static alignment onto the geometry
///
/// The strategy is as follows:
/// - The decorator is configured with a list of transform stores that
///   are valid for a given IOV range.
/// - The DD4hepGeometryContext is constructed for each event, however,
///   it basically only needs a delegate that points into the correct 
///   transform store.
///      - This delegate is handled with a simple (private) AlignmentStore struct
///      - If no valid delegate is found, it is created and stored 
///  - The Decorator keeps track of already used Alignment stores and reuses
///   them if they are still valid by simply resusing the connection.
///
/// The alignments are stored in a HierarchyMap in a hierarchical way
class DD4hepAlignmentDecorator : public IContextDecorator {
 public:
  /// @brief nested configuration struct
  struct Config {
    /// The alignment store map higher bound IOV range (event numbers)
    std::vector<std::tuple<std::array<std::size_t, 2u>,
                           std::shared_ptr<Acts::ITransformStore>>>
        transformStores;
    /// The nominal alignment store (before first bound, after last bound)
    std::shared_ptr<Acts::ITransformStore> nominalStore = nullptr;
  };

  /// Constructor
  ///
  /// @param cfg Configuration struct
  /// @param logger The logging framework
  explicit DD4hepAlignmentDecorator(
      const Config& cfg,
      std::unique_ptr<const Acts::Logger> logger = Acts::getDefaultLogger(
          "DD4hepAlignmentDecorator", Acts::Logging::INFO));

  /// Virtual destructor
  ~DD4hepAlignmentDecorator() override = default;

  /// @brief decorates (adds, modifies) the AlgorithmContext
  /// with a geometric rotation per event
  ///
  /// @note If decorators depend on each other, they have to be
  /// added in order.
  ///
  /// @param context the bare (or at least non-const) Event context
  ProcessCode decorate(AlgorithmContext& context) override;

  /// @brief decorator name() for screen output
  const std::string& name() const override { return m_name; }

 private:
  // The struct to cache the delegates
  struct AlignmentStore {
    /// Constructor from a transform store pointer
    /// @param transformStore the transform store
    /// @note The transform store is not owned by this class, this simply
    /// acts as a wrapper struct
    explicit AlignmentStore(const Acts::ITransformStore* transformStore)
        : m_transformStore(transformStore) {}

    const Acts::ITransformStore* m_transformStore = nullptr;
    /// Return the contextual transform for a given surface (from detector
    /// element)
    /// @param detElem the dd4hep detector element
    /// @return a Transform3 pointer if found, otherwise nullptr
    const Acts::Transform3* transform(
        const Acts::DD4hepDetectorElement& detElem) const {
      // Mockup implementation
      return m_transformStore->contextualTransform(detElem.surface());
    }
  };

  Config m_cfg;                                  ///< the configuration class
  std::unique_ptr<const Acts::Logger> m_logger;  ///!< the logging instance
  const std::string m_name = "DD4hepAlignmentDecorator";

  /// The cache
  std::vector<std::tuple<std::array<std::size_t, 2u>,
                         AlignmentStore>>  ///!< the alignment stores
      m_alignmentStores;           ///< the alignment stores

  /// Private access to the logging instance
  const Acts::Logger& logger() const { return *m_logger; }
};

}  // namespace ActsExamples
