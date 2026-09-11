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
#include "Acts/Utilities/MultiAxisSpec.hpp"

#include <iosfwd>
#include <optional>
#include <vector>

namespace Acts {

/// @addtogroup material
/// @{

/// @brief Proxy to SurfaceMaterial that carries the intended binning
///
/// The ProtoSurfaceMaterial class acts as a proxy to the SurfaceMaterial
/// to mark the layers and surfaces on which the material should be mapped on
/// at construction time of the geometry, and to hand over the granularity of
/// the material map.
///
/// The binning is a @c MultiAxisSpec2D whose axes are typically deferred, i.e.
/// they fix the number of bins and the direction but leave range and boundary
/// type to the surface the proto material sits on. Mapping resolves them with
/// @c resolveMultiAxis .
///
/// A proto material without binning marks a surface for homogeneous material.
class ProtoSurfaceMaterial final : public ISurfaceMaterial {
 public:
  /// Constructor without binning - marks the surface for homogeneous material
  ProtoSurfaceMaterial() = default;

  /// Constructor with a binning description
  ///
  /// @param binning the 2D binning description for the material map binning
  /// @param mappingType is the type of surface mapping associated to the surface
  explicit ProtoSurfaceMaterial(MultiAxisSpec2D binning,
                                MappingType mappingType = MappingType::Default)
      : ISurfaceMaterial(1., mappingType), m_binning(std::move(binning)) {}

  /// Scale operation - dummy implementation
  ///
  /// @return Reference to this object
  ProtoSurfaceMaterial& scale(double /*factor*/) final { return (*this); }

  /// Return the binning description
  /// @return the binning, unset for homogeneous proto material
  const std::optional<MultiAxisSpec2D>& binning() const { return m_binning; }

  /// Return method for full material description of the Surface - from local
  /// coordinates
  ///
  /// @return will return dummy material
  const MaterialSlab& materialSlab(const Vector2& /*lp*/) const final {
    return (m_materialSlab);
  }

  /// @copydoc ISurfaceMaterial::localAxisDirections() const
  ///
  /// @note Deliberately empty even when the binning carries directions: the
  ///       spec is resolved against the surface during mapping, in canonical
  ///       local axis order, so no axis swapping must be set up for the proxy
  ///       itself.
  std::vector<AxisDirection> localAxisDirections() const final { return {}; }

  /// Return method for full material description of the Surface - from the
  /// global coordinates
  ///
  /// @return will return dummy material
  /// @deprecated Use materialSlab(const Vector2&) with a prior
  ///             Surface::globalToLocal() call instead.
  [[deprecated(
      "Use materialSlab(const Vector2& lp) with a prior "
      "Surface::globalToLocal() call instead")]] const MaterialSlab&
  materialSlab(const Vector3& /*gp*/) const final {
    return (m_materialSlab);
  }

  using ISurfaceMaterial::materialSlab;

  /// Output Method for std::ostream
  ///
  /// @param sl is the output stream
  /// @return The output stream
  std::ostream& toStream(std::ostream& sl) const final;

 private:
  /// The binning description, unset for homogeneous proto material
  std::optional<MultiAxisSpec2D> m_binning = std::nullopt;

  /// Dummy material properties
  MaterialSlab m_materialSlab = MaterialSlab::Nothing();
};

/// @}

}  // namespace Acts
