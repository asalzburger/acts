// This file is part of the Acts project.
//
// Copyright (C) 2021 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Common.hpp"
#include "Acts/Experimental/VolumeLinks.hpp"
#include "Acts/Utilities/BinningType.hpp"
#include "Acts/Utilities/Helpers.hpp"
#include "Acts/Utilities/detail/Axis.hpp"

#include <functional>

namespace Acts {

template <typename axis_t>
struct BinnedLinkT {
  /// The equidistant axis
  axis_t axis;

  /// The binning value for the cast
  BinningValue bvalue = BinningValue::binX;

  /// The offset transform into local
  Transform3 toLocal = Transform3::Identity();

  /// Constructor without offset transforms
  ///
  /// @param axis_ is the axis
  /// @param bvalue_ is the binning value
  /// @param toLocal_ is the transform into local binning frame
  BinnedLinkT(axis_t&& axis_, BinningValue bvalue_ = binX,
              const Transform3& toLocal_ = Transform3::Identity())
      : axis(std::move(axis_)), bvalue(bvalue_), toLocal(toLocal_) {}

  /// Call operator
  ///
  /// @param transform into the binning frame
  /// @param position the position for the request
  unsigned int operator()(const Vector3& position) const {
    // The position in the local frame
    Vector3 posInFrame = toLocal * position;

    ActsScalar castedScalar = VectorHelpers::cast(posInFrame, bvalue);
    int bin = axis.getBin(castedScalar) - 1;
    return bin < 0 ? 0u
           : bin < static_cast<int>(axis.getNBins())
               ? static_cast<unsigned int>(bin)
               : static_cast<unsigned int>(axis.getNBins() - 1);
  }
};

using EquidistantVolumeLink = BinnedLinkT<detail::EquidistantAxis>;
using VariableVolumeLink = BinnedLinkT<detail::VariableAxis>;

}  // namespace Acts
