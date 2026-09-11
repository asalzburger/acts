// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Material/ISurfaceMaterial.hpp"
#include "Acts/Material/MaterialSlab.hpp"
#include "Acts/Utilities/BinUtility.hpp"
#include "Acts/Utilities/MultiAxisSpec.hpp"

#include <iosfwd>
#include <optional>

namespace Acts {

/// @brief Convert a bin utility into an equivalent 2D binning spec
///
/// Only the bin structure and the axis direction survive: range and boundary
/// type are dropped and left to the surface the spec is later resolved
/// against, which is where a proto binning took them from in the first place.
/// A one-dimensional bin utility is padded with a single bin along the
/// partner direction, which is how a 2D spec expresses binning restricted to
/// one local direction.
///
/// Azimuthal binning given as @c AxisPhi alongside @c AxisZ is normalised to
/// @c AxisRPhi , the canonical local axis of a cylinder. The binning is
/// geometrically the same; only the unit of the axis changes.
///
/// @param binUtility the bin utility to convert
/// @throws std::invalid_argument if the bin utility has more than two
///         dimensions, or a direction without a known partner
/// @return the equivalent deferred 2D binning spec, unset for a bin utility
///         without dimensions
///
/// @note Transitional. @c BinUtility , @c BinnedSurfaceMaterial and this
///       helper are retired together - new code should build a
///       @c MultiAxisSpec2D directly.
std::optional<MultiAxisSpec2D> binUtilityToMultiAxisSpec(
    const BinUtility& binUtility);

/// @brief Convert a 2D binning spec into an equivalent bin utility
///
/// The inverse of @c binUtilityToMultiAxisSpec , for the legacy json material
/// map format. A deferred axis contributes its bin structure and direction;
/// the range it leaves open is written as a placeholder that the reader drops
/// again.
///
/// @param binning the binning spec to convert
/// @return the equivalent bin utility
///
/// @note Transitional, see @c binUtilityToMultiAxisSpec .
BinUtility multiAxisSpecToBinUtility(const MultiAxisSpec2D& binning);

/// @ingroup material
///
/// It extends the @ref ISurfaceMaterial base class and is an array pf
/// MaterialSlab. This is not memory optimised as every bin
/// holds one material property object.
///
/// The split factors:
///    - 1. : oppositePre
///    - 0. : alongPre
class BinnedSurfaceMaterial : public ISurfaceMaterial {
 public:
  /// Explicit constructor with only full MaterialSlab,
  /// for one-dimensional binning.
  ///
  /// @param binUtility defines the binning structure on the surface (copied)
  /// @param materialVector is the vector of material slabs as recorded (moved)
  /// @param splitFactor is the pre/post splitting directive
  /// @param mappingType is the type of surface mapping associated to the surface
  BinnedSurfaceMaterial(const BinUtility& binUtility,
                        MaterialSlabVector materialVector,
                        double splitFactor = 0.,
                        MappingType mappingType = MappingType::Default);

  /// Explicit constructor with only full MaterialSlab,
  /// for two-dimensional binning.
  ///
  /// @param binUtility defines the binning structure on the surface (copied)
  /// @param materialMatrix is the matrix of material slabs as recorded (moved)
  /// @param splitFactor is the pre/post splitting directive
  /// @param mappingType is the type of surface mapping associated to the surface
  BinnedSurfaceMaterial(const BinUtility& binUtility,
                        MaterialSlabMatrix materialMatrix,
                        double splitFactor = 0.,
                        MappingType mappingType = MappingType::Default);

  /// Scale operation
  ///
  /// @param factor is the scale factor for the full material
  /// @return Reference to this object after scaling
  BinnedSurfaceMaterial& scale(double factor) final;

  /// Return the BinUtility
  /// @return Reference to the bin utility used for material binning
  const BinUtility& binUtility() const { return m_binUtility; }

  /// Return the binning as a 2D binning spec
  ///
  /// Lets consumers that have moved on to @c MultiAxisSpec2D - the material
  /// mapping in particular - re-map an existing binned map without having to
  /// know about @c BinUtility .
  ///
  /// @return the equivalent deferred binning spec, unset if unbinned
  std::optional<MultiAxisSpec2D> binningSpec() const {
    return binUtilityToMultiAxisSpec(m_binUtility);
  }

  /// @brief Retrieve the entire material slab matrix
  /// @return Reference to the complete matrix of material slabs
  const MaterialSlabMatrix& fullMaterial() const { return m_fullMaterial; }

  /// @copydoc ISurfaceMaterial::materialSlab(const Vector2&) const
  const MaterialSlab& materialSlab(const Vector2& lp) const final;

  /// @copydoc ISurfaceMaterial::materialSlab(const Vector3&) const
  /// @deprecated Use materialSlab(const Vector2&) with a prior
  ///             Surface::globalToLocal() call instead.
  [[deprecated(
      "Use materialSlab(const Vector2& lp) with a prior "
      "Surface::globalToLocal() call instead")]] const MaterialSlab&
  materialSlab(const Vector3& gp) const final;

  using ISurfaceMaterial::materialSlab;

  /// @copydoc ISurfaceMaterial::localAxisDirections() const
  std::vector<AxisDirection> localAxisDirections() const final;

  /// Output Method for std::ostream, to be overloaded by child classes
  /// @param sl The output stream to write to
  /// @return Reference to the output stream after writing
  std::ostream& toStream(std::ostream& sl) const final;

 private:
  /// The helper for the bin finding
  BinUtility m_binUtility;

  /// The five different MaterialSlab
  MaterialSlabMatrix m_fullMaterial;
};

}  // namespace Acts
