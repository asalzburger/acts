// This file is part of the Acts project.
//
// Copyright (C) 2022 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Utilities/BinningData.hpp"
#include "Acts/Utilities/Enumerate.hpp"
#include "Acts/Utilities/Helpers.hpp"

#include <algorithm>
#include <array>
#include <vector>

#include <boost/icl/type_traits/is_container.hpp>
#include <boost/type_traits/is_list_constructible.hpp>

/// @note this should go into the 'Utilities' module

namespace Acts {

/// Enable return of an entry to its the same as return type
///
/// @param entry is the grid entry and is unmodified on @return
///
/// @todo check if we can use const reference return here
template <
    typename entry_type, typename return_type = entry_type,
    std::enable_if_t<std::is_same_v<entry_type, return_type>, bool> = true>
const return_type& convert_entry(const entry_type& entry) {
  return entry;
}

/// Enable return to container for integer type entries with initializer
/// list construction
///
/// @param entry is an integer type entry and a container will be created
/// with initializer list on @return
template <typename entry_type, typename return_type,
          std::enable_if_t<
              std::conjunction_v<std::is_integral<entry_type>,
                                 boost::is_list_constructible<return_type>,
                                 boost::icl::is_container<return_type>>,
              bool> = true>
return_type convert_entry(const entry_type& entry) {
  return {entry};
}

/// Enable return to container from a different container
///
/// @param entry is an container type entry a return container
/// type will be filled for @return
template <
    typename entry_type, typename return_type,
    std::enable_if_t<
        std::conjunction_v<std::negation<std::is_same<entry_type, return_type>>,
                           boost::icl::is_container<entry_type>,
                           boost::icl::is_container<return_type>>,
        bool> = true>
return_type convert_entry(const entry_type& entry) {
  return_type r(entry.begin(), entry.end());
  return r;
}

/// Bin Filling and neighborhood resolving

/// Provide only the bin
struct BinOnly {
  /// Return only the bin entry w/o neighbors into and convert it into the
  /// return type container.
  ///
  /// @tparam grid_t the type of the grid with its broadcased point
  /// @tparam return_type_t the type type of the return object (converted)
  ///
  /// @param grid the grid for the access
  /// @param gpos the grid possition access
  ///
  /// @return the bin entry in a return type format
  template <typename grid_t, typename return_type_t>
  return_type_t fill(const grid_t& grid, typename grid_t::point_t& gpos) const {
    return convert_entry<typename grid_t::value_type, return_type_t>(
        grid.atPosition(gpos));
  }
};

/// An inserter struct for Set types
struct SetTypeInserter {
  /// Call insert operator to insert into a set type output container
  ///
  /// @tparam input_type_t the input type container
  /// @tparam output_type_t the output type container
  ///
  /// @param inputContinaer the entries from the bin
  /// @param outputContainer the final return type container
  ///
  template <typename input_type_t, typename return_type_t>
  void operator()(const input_type_t& inputContainer,
                  return_type_t& outputContainer) const {
    // Insert using set type semantics
    outputContainer.insert(inputContainer.begin(), inputContainer.end());
  }
};

/// An inserter struct for Unordered Set types
struct UnorderedSetTypeInserter {
  /// Call insert operator to insert into a set type output container
  ///
  /// @tparam input_type_t the input type container
  /// @tparam output_type_t the output type container
  ///
  /// @param inputContinaer the entries from the bin
  /// @param outputContainer the final return type container
  ///
  template <typename input_type_t, typename return_type_t>
  void operator()(const input_type_t& inputContainer,
                  return_type_t& outputContainer) const {
    // Insert using unordered set type semantics
    for (const auto& i : inputContainer) {
      outputContainer.insert(outputContainer.end(), i);
    }
  }
};

/// An inserter struct for vector types
template <bool kSORT = false>
struct VectorTypeInserter {
  /// Call insert operator to insert into a vector type output container
  ///
  /// @tparam input_type_t the input type container
  /// @tparam output_type_t the output type container
  ///
  /// @param inputContinaer the entries from the bin
  /// @param outputContainer the final return type container
  ///
  /// @note sorting and uniqueness can be guranteed setting @tparam kSORT
  template <typename input_type_t, typename return_type_t>
  void operator()(const input_type_t& inputContainer,
                  return_type_t& outputContainer) const {
    // Use vector type ::insert(...) semantics
    outputContainer.insert(outputContainer.end(), inputContainer.begin(),
                           inputContainer.end());
    // Sort & remove duplicates if needed
    if (kSORT) {
      std::sort(outputContainer.begin(), outputContainer.end());
      auto last = std::unique(outputContainer.begin(), outputContainer.end());
      outputContainer.erase(last, outputContainer.end());
    }
  }
};

template <unsigned int kN = 1u, typename inserter_t = VectorTypeInserter<>>
struct SymmetricNeighbors {
  inserter_t inserter;

  /// @tparam grid_t the type of the grid with its broadcased point
  /// @tparam return_type_t the type type of the return object (converted)
  ///
  /// @param grid the grid for the access
  /// @param gpos the grid possition access
  ///
  /// @return the bin entry in a return type format
  template <typename grid_t, typename return_type_t,
            std::enable_if_t<boost::icl::is_container<return_type_t>::value,
                             bool> = true>
  return_type_t fill(const grid_t& grid, typename grid_t::point_t& gpos) const {
    // Get central Index and neighbors around and fill into the unique set
    auto binIndex = grid.localBinsFromPosition(gpos);
    return_type_t returnContainer;
    // Fill index with neighbors
    auto neighborIndices = grid.neighborHoodIndices(binIndex, kN).collect();
    for (const auto& neighborIndex : neighborIndices) {
      auto neighborContainer =
          convert_entry<typename grid_t::value_type, return_type_t>(
              grid.at(neighborIndex));
      // Use the dedicated inserter for the return container
      inserter(neighborContainer, returnContainer);
    }
    return returnContainer;
  }
};

/// A Grid based index link implementation based on a a @tparam grid_t
/// that returns a
///
/// It allows if necessary to turn the grid
/// entry into a different formal by the @tparam provider_t template
///
/// @note that neighbor indices are not filled
template <typename grid_t,
          typename return_container_t = typename grid_t::value_type>
struct GridEntryImpl {
  using return_type = return_container_t;

  /// The grid
  grid_t grid;

  /// The parameter casts from local into grid point definition
  std::vector<BinningValue> parameterCasts = {};

  /// The transform into grid local frame
  Transform3 toLocal = Transform3::Identity();

  /// Constructor
  ///
  /// @param grid_ the access grid for indices
  /// @param parameterCasts_ the binning value list for casting parameters
  ///        into the grid point definition
  /// @param toLocal_ the transform to local for the grid access
  ///
  GridEntryImpl(const grid_t& grid_,
                const std::vector<BinningValue>& parameterCasts_,
                const Transform3& toLocal_ = Transform3::Identity())
      : grid(grid_), parameterCasts(parameterCasts_), toLocal(toLocal_) {}

  /// Constructor
  ///
  /// @param grid_ the access grid for indices
  /// @param parameterCasts_ the binning value list for casting parameters
  ///        into the grid point definition
  /// @param toLocal_ the transform to local for the grid access
  ///
  GridEntryImpl(grid_t&& grid_,
                const std::vector<BinningValue>& parameterCasts_,
                const Transform3& toLocal_ = Transform3::Identity())
      : grid(std::move(grid_)),
        parameterCasts(parameterCasts_),
        toLocal(toLocal_) {}

  /// Ask for the link(s) given a position, allowing to provide
  /// a different filler for the bin access
  ///
  /// @tparam filler_t is the filler type as defined above
  ///
  /// @param position is the position for the link request
  ///
  /// @return the link(s) given by the provider return type format
  template <typename filler_t>
  return_type links(const Vector3& position) const {
    // Bring the position into local frame & cast into the grid point definition
    Vector3 posInFrame = toLocal * position;
    typename decltype(grid)::point_t gposition;
    for (auto [i, castValue] : enumerate(parameterCasts)) {
      gposition[i] = VectorHelpers::cast(posInFrame, castValue);
    }
    // Return the filled & converted return type
    return filler_t().template fill<decltype(grid), return_type>(grid,
                                                                 gposition);
  }

  /// Direct access to the bin content, does not need memory allocation
  ///
  /// @param position is the position for the link request
  ///
  const typename grid_t::value_type& links(const Vector3& position) const {
    // Bring the position into local frame & cast into the grid point definition
    Vector3 posInFrame = toLocal * position;
    typename decltype(grid)::point_t gposition;
    for (auto [i, castValue] : enumerate(parameterCasts)) {
      gposition[i] = VectorHelpers::cast(posInFrame, castValue);
    }
    return grid.atPosition(gposition);
  }

  /// Connect with adjecent bins, i.e. also fill differring neighborhood
  /// bin values into the bin, in order to spare repeated neighborhood
  /// requests
  ///
  /// @note this is only enabled if the grid_t::value_type is of container
  /// type to allow additional assignments
  ///
  template <typename filler_t>
  void connectAdjacent() {
    filler_t filler;
    // We make a copy of the grid & fill it with the adjecent-connected bins
    grid_t agrid = grid;
    for (size_t g = 0; g < grid.size(); ++g) {
      auto localBins = grid.localBinsFromGlobalBin(g);
      typename grid_t::point_t gpoint = grid.binCenter(localBins);
      agrid.atLocalBins(localBins) =
          filler.template fill<decltype(grid), typename grid_t::value_type>(
              grid, gpoint);
    }
    // re-assign to the storage grid
    grid = agrid;
  }
};

}  // namespace Acts
