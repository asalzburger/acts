

// This file is part of the Acts project.
//
// Copyright (C) 2022 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "Acts/Experimental/GridBinningDetection.hpp"

#include "Acts/Utilities/Enumerate.hpp"

#include <iostream>
#include <tuple>
#include <vector>

std::tuple<size_t, std::vector<Acts::ActsScalar>>
Acts::GridBinningDetection::operator()(std::vector<ActsScalar>& values) {
  // Sort & make unique
  std::sort(values.begin(), values.end());
  auto last = std::unique(values.begin(), values.end());
  values.erase(last, values.end());

  // The local cluster definition
  using Cluster = std::pair<ActsScalar, std::vector<ActsScalar>>;

  /// Helper method to add to the cluster (poor man's clustering)
  ///
  /// @param c the cluster
  /// @param v the value
  auto addToCluster = [](Cluster& c, ActsScalar v) -> void {
    c.second.push_back(v);
    ActsScalar cc = 0.;
    unsigned int nc = 0;
    for (const auto& cv : c.second) {
      cc += cv;
      ++nc;
    }
    cc *= 1. / nc;
    c.first = cc;
  };

  /// Helper method to cluster the values
  ///
  /// @param cvalues the input values
  ///
  /// @return a list of clusters
  auto clusterize =
      [&](const std::vector<ActsScalar>& cvalues) -> std::vector<Cluster> {
    std::vector<Cluster> vclusters;
    for (const auto& v : cvalues) {
      auto cl =
          std::find_if(vclusters.begin(), vclusters.end(), [&](const auto& t) {
            return (std::abs(t.first - v) < clusterTolerance);
          });
      if (cl != vclusters.end()) {
        addToCluster(*cl, v);
      } else {
        vclusters.push_back(Cluster{v, {v}});
      }
    }
    return vclusters;
  };

  // Create the clusters of bin boundaries
  auto boundaryClusters = clusterize(values);

  // Catch weird 1 bin case
  if (boundaryClusters.size() == 1u) {
    return {1u, values};
  }

  // Fill the return values in this case
  std::vector<ActsScalar> boundaries;
  std::vector<ActsScalar> binWidths;
  // Save reserve in case of phi wrapping insertion
  boundaries.reserve(boundaryClusters.size() + 1u);
  binWidths.reserve(boundaryClusters.size());
  ActsScalar lastCluster = 0.;
  for (auto [i, cluster] : enumerate(boundaryClusters)) {
    boundaries.push_back(cluster.first);
    if (i > 0u) {
      binWidths.push_back(std::abs(cluster.first - lastCluster));
    }
    lastCluster = cluster.first;
  }

  // Let's check for phi wrapping and insert the missing value
  if (checkPhiWrapping and boundaryClusters[0].first < 0. and
      lastCluster > 0.) {
    ActsScalar negSideDiff = std::abs(-M_PI - boundaryClusters[0].first);
    ActsScalar posSideDiff = std::abs(M_PI - lastCluster);
    // If the clustering has yielded to very small values on the negative
    // and positive side, pull together to the common average (either way)
    if (negSideDiff < clusterTolerance and posSideDiff < clusterTolerance) {
      // Force to M_PI/-M_PI boundaries, should also be safe for 2 bin entry
      boundaries[0] = -M_PI;
      boundaries[boundaries.size() - 1u] = M_PI;
      // Force the bin widths
      binWidths[0] = boundaries[1] - boundaries[0];
      binWidths[boundaries.size() - 1u] = boundaries[boundaries.size() - 1u] -
                                          boundaries[boundaries.size() - 2u];

    } else if (negSideDiff > 0. and posSideDiff > 0.) {
      // This is the case when negative side and/or positive side might have
      // fallen off the cliff, we insert two "fakeish" bins
      if (std::abs(negSideDiff - posSideDiff) < clusterTolerance) {
        // Symmetric fall-off around M_PI boundary,
        // insert the exact +/- M_PI boundary, and let the
        // bin multiplication figure it out
        boundaries.insert(boundaries.begin(), -M_PI);
        binWidths.insert(binWidths.begin(),
                         std::abs(-M_PI - boundaryClusters[0].first));
        boundaries.push_back(M_PI);
        binWidths.push_back(M_PI - lastCluster);
      } else if (negSideDiff > posSideDiff) {
        // Insert a fake lower value, this claims the bin
        ActsScalar fakeLow = -M_PI - posSideDiff;
        boundaries.insert(boundaries.begin(), fakeLow);
        binWidths.insert(binWidths.begin(),
                         std::abs(fakeLow - boundaryClusters[0].first));
      } else {
        // Insert a fake higher value, this claims the bin
        ActsScalar fakeHigh = M_PI + posSideDiff;
        boundaries.push_back(fakeHigh);
        binWidths.push_back(fakeHigh - lastCluster);
      }
    }
  }

  // Cluster the bin steps & check for sub-binning
  auto binWidthClusters = clusterize(binWidths);
  // Equidistant binning detected
  if (binWidthClusters.size() == 1u) {
    // Return the number of size
    return {binWidths.size(), {}};
  } else if (binWidthClusters.size() == 2u) {
    // Only one type of sub binning is allowd
    // Get the ratio between the bin widths
    ActsScalar binWidth0 = binWidthClusters[0].first;
    ActsScalar binWidth1 = binWidthClusters[1].first;
    // Build the ratio and check for close to integer binning
    ActsScalar binWidthRatio =
        binWidth0 > binWidth1 ? binWidth0 / binWidth1 : binWidth1 / binWidth0;
    if ((std::round(binWidthRatio) - binWidthRatio) / binWidthRatio <
        relSubEqTolerance) {
      unsigned int nBins =
          std::round((values[values.size() - 1u] - values[0u]) /
                     std::min(binWidth0, binWidth1));
      return {nBins, {}};
    }
  }
  // Variable binning return
  return {binWidths.size(), boundaries};
}