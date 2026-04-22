// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Material/AccumulatedSurfaceMaterial.hpp"

#include "Acts/Material/BinnedSurfaceMaterial.hpp"
#include "Acts/Material/HomogeneousSurfaceMaterial.hpp"
#include "Acts/Utilities/ProtoAxisHelpers.hpp"

#include <array>
#include <stdexcept>
#include <utility>

namespace {

std::array<std::size_t, 2u> binCounts(
    const std::vector<Acts::DirectedProtoAxis>& dProtoAxes) {
  const std::size_t bins0 = dProtoAxes[0].getAxis().getNBins();
  const std::size_t bins1 =
      dProtoAxes.size() > 1 ? dProtoAxes[1].getAxis().getNBins() : 1u;
  return {bins0, bins1};
}

std::array<std::size_t, 3u> binTriple(
    const std::vector<Acts::DirectedProtoAxis>& dProtoAxes,
    const Acts::Vector3& gp) {
  const std::size_t b0 =
      Acts::ProtoAxisHelpers::binFromProtoAxis(dProtoAxes[0], gp);
  const std::size_t b1 =
      dProtoAxes.size() > 1
          ? Acts::ProtoAxisHelpers::binFromProtoAxis(dProtoAxes[1], gp)
          : 0u;
  return {b0, b1, 0u};
}

}  // namespace

// Default Constructor - for homogeneous material
Acts::AccumulatedSurfaceMaterial::AccumulatedSurfaceMaterial(double splitFactor)
    : m_splitFactor(splitFactor) {
  AccumulatedVector accMat = {{AccumulatedMaterialSlab()}};
  m_accumulatedMaterial = {{accMat}};
}

// Binned Material constructor with split factor
Acts::AccumulatedSurfaceMaterial::AccumulatedSurfaceMaterial(
    const std::vector<DirectedProtoAxis>& dProtoAxes, double splitFactor)
    : m_directedProtoAxes(dProtoAxes), m_splitFactor(splitFactor) {
  const auto [bins0, bins1] = binCounts(m_directedProtoAxes);
  AccumulatedVector accVec(bins0, AccumulatedMaterialSlab());
  m_accumulatedMaterial = AccumulatedMatrix(bins1, accVec);
}

std::array<std::size_t, 3> Acts::AccumulatedSurfaceMaterial::accumulate(
    const Vector3& gp, const MaterialSlab& mp, double pathCorrection) {
  if (m_directedProtoAxes.empty()) {
    m_accumulatedMaterial[0][0].accumulate(mp, pathCorrection);
    return {0, 0, 0};
  }

  const auto [b0, b1, b2] = binTriple(m_directedProtoAxes, gp);
  m_accumulatedMaterial[b1][b0].accumulate(mp, pathCorrection);
  return {b0, b1, b2};
}

// Void average for vacuum assignment
void Acts::AccumulatedSurfaceMaterial::trackVariance(const Vector3& gp,
                                                     MaterialSlab slabReference,
                                                     bool emptyHit) {
  if (m_directedProtoAxes.empty()) {
    m_accumulatedMaterial[0u][0u].trackVariance(slabReference, emptyHit);
    return;
  }

  std::vector<std::array<std::size_t, 3>> trackBins = {
      binTriple(m_directedProtoAxes, gp)};
  trackVariance(trackBins, slabReference);
}

// Average the information accumulated during one event
void Acts::AccumulatedSurfaceMaterial::trackVariance(
    const std::vector<std::array<std::size_t, 3>>& trackBins,
    MaterialSlab slabReference, bool emptyHit) {
  // the homogeneous material case
  if (m_directedProtoAxes.empty()) {
    m_accumulatedMaterial[0][0].trackVariance(slabReference, emptyHit);
    return;
  }
  // The touched bins are known, so you can access them directly
  if (!trackBins.empty()) {
    for (auto bin : trackBins) {
      m_accumulatedMaterial[bin[1]][bin[0]].trackVariance(slabReference);
    }
  } else {
    // Touched bins are not known: Run over all bins
    for (auto& matVec : m_accumulatedMaterial) {
      for (auto& mat : matVec) {
        mat.trackVariance(slabReference);
      }
    }
  }
}

// Void average for vacuum assignment
void Acts::AccumulatedSurfaceMaterial::trackAverage(const Vector3& gp,
                                                    bool emptyHit) {
  if (m_directedProtoAxes.empty()) {
    m_accumulatedMaterial[0][0].trackAverage(emptyHit);
    return;
  }

  trackAverage(std::vector<std::array<std::size_t, 3>>{binTriple(
                   m_directedProtoAxes, gp)},
               emptyHit);
}

// Average the information accumulated during one event
void Acts::AccumulatedSurfaceMaterial::trackAverage(
    const std::vector<std::array<std::size_t, 3>>& trackBins, bool emptyHit) {
  // the homogeneous material case
  if (m_directedProtoAxes.empty()) {
    m_accumulatedMaterial[0][0].trackAverage(emptyHit);
    return;
  }

  // The touched bins are known, so you can access them directly
  if (!trackBins.empty()) {
    for (auto bin : trackBins) {
      m_accumulatedMaterial[bin[1]][bin[0]].trackAverage(emptyHit);
    }
  } else {
    // Touched bins are not known: Run over all bins
    for (auto& matVec : m_accumulatedMaterial) {
      for (auto& mat : matVec) {
        mat.trackAverage(emptyHit);
      }
    }
  }
}

/// Total average creates SurfaceMaterial
std::unique_ptr<const Acts::ISurfaceMaterial>
Acts::AccumulatedSurfaceMaterial::totalAverage() {
  if (m_directedProtoAxes.empty()) {
    // Return HomogeneousSurfaceMaterial
    return std::make_unique<HomogeneousSurfaceMaterial>(
        m_accumulatedMaterial[0][0].totalAverage().first, m_splitFactor);
  }
  const auto [bins0, bins1] = binCounts(m_directedProtoAxes);

  // Create the properties matrix
  MaterialSlabMatrix mpMatrix(
      bins1, MaterialSlabVector(bins0, MaterialSlab::Nothing()));
  // Loop over and fill
  for (std::size_t ib1 = 0; ib1 < bins1; ++ib1) {
    for (std::size_t ib0 = 0; ib0 < bins0; ++ib0) {
      mpMatrix[ib1][ib0] = m_accumulatedMaterial[ib1][ib0].totalAverage().first;
    }
  }
  // Now return the BinnedSurfaceMaterial
  if (m_directedProtoAxes.size() == 1u) {
    return std::make_unique<const BinnedSurfaceMaterial>(
        m_directedProtoAxes[0], std::move(mpMatrix[0]), m_splitFactor);
  }
  if (m_directedProtoAxes.size() == 2u) {
    std::array<DirectedProtoAxis, 2u> axes = {m_directedProtoAxes[0],
                                              m_directedProtoAxes[1]};
    return std::make_unique<const BinnedSurfaceMaterial>(
        axes, std::move(mpMatrix), m_splitFactor);
  }
  throw std::invalid_argument(
      "AccumulatedSurfaceMaterial supports only 1D or 2D directed proto axes.");
}
