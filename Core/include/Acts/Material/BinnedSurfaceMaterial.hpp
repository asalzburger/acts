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
#include "Acts/Utilities/ProtoAxis.hpp"
#include "Acts/Utilities/VectorHelpers.hpp"

#include <array>
#include <iosfwd>

namespace Acts {

/// @ingroup material
///
/// It extends the @ref ISurfaceMaterial base class and is an array pf
/// MaterialSlab. This is not memory optimised as every bin
/// holds one material property object.
class BinnedSurfaceMaterial : public ISurfaceMaterial {
 public:
  /// Default Constructor - deleted
  BinnedSurfaceMaterial() = delete;

  /// Explicit constructor with only full MaterialSlab,
  /// for one-dimensional binning.
  ///
  /// The split factors:
  ///    - 1. : oppositePre
  ///    - 0. : alongPre
  ///  ===> 1 Dimensional array
  ///
  /// @param dProtoAxis defines the binning structure on the surface
  /// @param fullProperties is the vector of properties as recorded (moved)
  /// @param splitFactor is the pre/post splitting directive
  /// @param mappingType is the type of surface mapping associated to the surface
  BinnedSurfaceMaterial(const DirectedProtoAxis& dProtoAxis,
                        MaterialSlabVector fullProperties,
                        double splitFactor = 0.,
                        MappingType mappingType = MappingType::Default);

  /// Explicit constructor with only full MaterialSlab,
  /// for two-dimensional binning.
  ///
  /// The split factors:
  ///    - 1. : oppositePre
  ///    - 0. : alongPre
  ///  ===> 1 Dimensional array
  ///
  /// @param dProtoAxes defines the binning structure on the surface
  /// @param fullProperties is the vector of properties as recorded (moved)
  /// @param splitFactor is the pre/post splitting directive
  /// @param mappingType is the type of surface mapping associated to the surface
  BinnedSurfaceMaterial(const std::array<DirectedProtoAxis, 2>& dProtoAxes,
                        MaterialSlabMatrix fullProperties,
                        double splitFactor = 0.,
                        MappingType mappingType = MappingType::Default);

  /// Copy Move Constructor
  ///
  /// @param bsm is the source object to be copied
  BinnedSurfaceMaterial(BinnedSurfaceMaterial&& bsm) = default;

  /// Copy Constructor
  ///
  /// @param bsm is the source object to be copied
  BinnedSurfaceMaterial(const BinnedSurfaceMaterial& bsm) = default;

  /// Assignment Move operator
  /// @param bsm The source object to move from
  /// @return Reference to this object after move assignment
  BinnedSurfaceMaterial& operator=(BinnedSurfaceMaterial&& bsm) = default;

  /// Assignment operator
  /// @param bsm The source object to copy from
  /// @return Reference to this object after copy assignment
  BinnedSurfaceMaterial& operator=(const BinnedSurfaceMaterial& bsm) = default;

  /// Destructor
  ~BinnedSurfaceMaterial() override = default;

  /// Scale operation
  ///
  /// @param factor is the scale factor for the full material
  /// @return Reference to this object after scaling
  BinnedSurfaceMaterial& scale(double factor) final;

  /// Return the directed proto axes
  /// @return Reference to the directed proto axes used for material binning
  const std::vector<DirectedProtoAxis>& directedProtoAxes() const;

  /// @brief Retrieve the entire material slab matrix
  /// @return Reference to the complete matrix of material slabs
  const MaterialSlabMatrix& fullMaterial() const;

  /// @copydoc ISurfaceMaterial::materialSlab(const Vector2&) const
  const MaterialSlab& materialSlab(const Vector2& lp) const final;

  /// @copydoc ISurfaceMaterial::materialSlab(const Vector3&) const
  const MaterialSlab& materialSlab(const Vector3& gp) const final;

  using ISurfaceMaterial::materialSlab;

  /// Output Method for std::ostream, to be overloaded by child classes
  /// @param sl The output stream to write to
  /// @return Reference to the output stream after writing
  std::ostream& toStream(std::ostream& sl) const final;

 private:
  /// The helper for the bin finding
  std::vector<DirectedProtoAxis> m_directedProtoAxes;

  /// The five different MaterialSlab
  MaterialSlabMatrix m_fullMaterial;
};

inline const std::vector<DirectedProtoAxis>&
BinnedSurfaceMaterial::directedProtoAxes() const {
  return m_directedProtoAxes;
}

inline const MaterialSlabMatrix& BinnedSurfaceMaterial::fullMaterial() const {
  return m_fullMaterial;
}

}  // namespace Acts
