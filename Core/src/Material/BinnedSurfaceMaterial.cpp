// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Material/BinnedSurfaceMaterial.hpp"

#include "Acts/Material/MaterialSlab.hpp"
#include "Acts/Utilities/AxisDefinitions.hpp"
#include "Acts/Utilities/AxisSpec.hpp"

#include <algorithm>
#include <array>
#include <ostream>
#include <stdexcept>
#include <utility>
#include <vector>

namespace Acts {

namespace {

/// @brief The partner of a local axis direction on the same surface
///
/// A one-dimensional binning only names one of the two local directions; the
/// other one is implied by the surface type the direction belongs to.
AxisDirection partnerDirection(AxisDirection direction) {
  using enum AxisDirection;
  switch (direction) {
    case AxisX:
      return AxisY;
    case AxisY:
      return AxisX;
    case AxisR:
      return AxisPhi;
    case AxisPhi:
      return AxisR;
    case AxisRPhi:
      return AxisZ;
    case AxisZ:
      return AxisRPhi;
    default:
      throw std::invalid_argument(
          "binUtilityToMultiAxisSpec: no partner local axis direction known "
          "for " +
          axisDirectionName(direction));
  }
}

/// @brief Convert one binning data into a deferred axis spec
AxisSpec deferredSpecFromBinningData(const BinningData& binningData,
                                     AxisDirection direction) {
  if (binningData.type == equidistant) {
    return AxisSpec::DeferredEquidistant(binningData.bins(), direction);
  }
  const std::vector<float>& edges = binningData.boundaries();
  std::vector<double> normalizedEdges(edges.size());
  const double min = edges.front();
  const double span = static_cast<double>(edges.back()) - min;
  std::ranges::transform(edges, normalizedEdges.begin(), [&](float edge) {
    return (static_cast<double>(edge) - min) / span;
  });
  normalizedEdges.front() = 0.;
  normalizedEdges.back() = 1.;
  return AxisSpec::DeferredVariable(std::move(normalizedEdges), std::nullopt,
                                    direction);
}

}  // namespace

std::optional<MultiAxisSpec2D> binUtilityToMultiAxisSpec(
    const BinUtility& binUtility) {
  const std::vector<BinningData>& binningData = binUtility.binningData();
  if (binningData.empty()) {
    return std::nullopt;
  }
  if (binningData.size() > 2u) {
    throw std::invalid_argument(
        "binUtilityToMultiAxisSpec: a surface binning of more than two "
        "dimensions cannot be expressed as a surface grid.");
  }

  // Azimuthal binning on a cylinder is canonically rPhi, not phi
  auto direction = [&](std::size_t i) {
    AxisDirection dir = binningData[i].binvalue;
    if (dir == AxisDirection::AxisPhi && binningData.size() == 2u &&
        binningData[1u - i].binvalue == AxisDirection::AxisZ) {
      return AxisDirection::AxisRPhi;
    }
    return dir;
  };

  AxisSpec spec0 = deferredSpecFromBinningData(binningData[0], direction(0));
  if (binningData.size() == 2u) {
    return MultiAxisSpec2D(
        {std::move(spec0),
         deferredSpecFromBinningData(binningData[1], direction(1))});
  }
  // Pad the unbinned local direction with a single bin
  return MultiAxisSpec2D(
      {std::move(spec0),
       AxisSpec::DeferredEquidistant(1u, partnerDirection(direction(0)))});
}

BinUtility multiAxisSpecToBinUtility(const MultiAxisSpec2D& binning) {
  BinUtility binUtility;
  for (const AxisSpec& spec : binning.axisSpecs()) {
    const BinningOption option =
        spec.boundaryType() == AxisBoundaryType::Closed ? closed : open;
    // A deferred axis has no range of its own; the placeholder is dropped
    // again by binUtilityToMultiAxisSpec on the way back
    const AxisDirection direction =
        spec.direction().value_or(AxisDirection::AxisX);
    if (spec.isEquidistant()) {
      const auto& params = spec.asEquidistant();
      binUtility += BinUtility(
          params.nBins, static_cast<float>(params.min.value_or(0.)),
          static_cast<float>(params.max.value_or(1.)), option, direction);
      continue;
    }
    const std::vector<double>& edges =
        spec.isDeferredVariable() ? spec.asDeferredVariable().normalizedEdges
                                  : spec.asVariable().edges;
    std::vector<float> fEdges(edges.size());
    std::ranges::transform(edges, fEdges.begin(),
                           [](double e) { return static_cast<float>(e); });
    binUtility += BinUtility(fEdges, option, direction);
  }
  return binUtility;
}

BinnedSurfaceMaterial::BinnedSurfaceMaterial(const BinUtility& binUtility,
                                             MaterialSlabVector materialVector,
                                             double splitFactor,
                                             MappingType mappingType)
    : ISurfaceMaterial(splitFactor, mappingType), m_binUtility(binUtility) {
  if (binUtility.dimensions() != 1) {
    throw std::invalid_argument(
        "BinnedSurfaceMaterial with material vector only supports 1D binning.");
  }
  if (binUtility.binningData()[0].bins() != materialVector.size()) {
    throw std::invalid_argument(
        "BinnedSurfaceMaterial: number of material bins does not match the "
        "number of provided material slabs.");
  }
  m_fullMaterial.push_back(std::move(materialVector));
}

BinnedSurfaceMaterial::BinnedSurfaceMaterial(const BinUtility& binUtility,
                                             MaterialSlabMatrix materialMatrix,
                                             double splitFactor,
                                             MappingType mappingType)
    : ISurfaceMaterial(splitFactor, mappingType),
      m_binUtility(binUtility),
      m_fullMaterial(std::move(materialMatrix)) {
  if (binUtility.dimensions() != 1 && binUtility.dimensions() != 2) {
    throw std::invalid_argument(
        "BinnedSurfaceMaterial with material matrix only supports 1D and 2D "
        "binning.");
  }
  if (binUtility.dimensions() == 1) {
    if (m_fullMaterial.size() != 1) {
      throw std::invalid_argument(
          "BinnedSurfaceMaterial with material matrix only supports 1D binning "
          "if the material matrix has exactly one row.");
    }
    if (binUtility.binningData()[0].bins() != m_fullMaterial[0].size()) {
      throw std::invalid_argument(
          "BinnedSurfaceMaterial: number of material bins does not match the "
          "number of provided material slabs.");
    }
  } else if (binUtility.dimensions() == 2) {
    if (binUtility.binningData()[1].bins() != m_fullMaterial.size()) {
      throw std::invalid_argument(
          "BinnedSurfaceMaterial: number of material bins in the first "
          "dimension does not match the number of provided material rows.");
    }
    for (const auto& materialVector : m_fullMaterial) {
      if (binUtility.binningData()[0].bins() != materialVector.size()) {
        throw std::invalid_argument(
            "BinnedSurfaceMaterial: number of material bins in the second "
            "dimension does not match the number of provided material slabs in "
            "each row.");
      }
    }
  }
}

BinnedSurfaceMaterial& BinnedSurfaceMaterial::scale(double factor) {
  for (auto& materialVector : m_fullMaterial) {
    for (auto& materialBin : materialVector) {
      materialBin.scaleThickness(factor);
    }
  }
  return *this;
}

const MaterialSlab& BinnedSurfaceMaterial::materialSlab(
    const Vector2& lp) const {
  const std::size_t ibin0 = m_binUtility.bin(lp[0], 0);
  const std::size_t ibin1 = m_binUtility.bin(lp[1], 1);
  return m_fullMaterial[ibin1][ibin0];
}

std::vector<AxisDirection> BinnedSurfaceMaterial::localAxisDirections() const {
  std::vector<AxisDirection> axisDirs;
  for (const auto& bd : m_binUtility.binningData()) {
    axisDirs.push_back(bd.binvalue);
  }
  return axisDirs;
}

const MaterialSlab& BinnedSurfaceMaterial::materialSlab(
    const Vector3& gp) const {
  const std::size_t ibin0 = m_binUtility.bin(gp, 0);
  const std::size_t ibin1 = m_binUtility.bin(gp, 1);
  return m_fullMaterial[ibin1][ibin0];
}

std::ostream& BinnedSurfaceMaterial::toStream(std::ostream& sl) const {
  sl << "BinnedSurfaceMaterial : " << std::endl;
  sl << "   - Number of Material bins [0,1] : " << m_binUtility.max(0) + 1
     << " / " << m_binUtility.max(1) + 1 << std::endl;
  sl << "   - Parse full update material    : " << std::endl;  //
  // output  the full material
  unsigned int imat1 = 0;
  for (auto& materialVector : m_fullMaterial) {
    unsigned int imat0 = 0;
    // the vector iterator
    for (auto& materialBin : materialVector) {
      sl << " Bin [" << imat1 << "][" << imat0 << "] - " << (materialBin);
      ++imat0;
    }
    ++imat1;
  }
  sl << "  - BinUtility: " << m_binUtility << std::endl;
  return sl;
}

}  // namespace Acts
