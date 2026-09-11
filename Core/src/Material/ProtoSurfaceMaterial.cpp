// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Material/ProtoSurfaceMaterial.hpp"

#include <ostream>

namespace Acts {

std::ostream& ProtoSurfaceMaterial::toStream(std::ostream& sl) const {
  sl << "Acts::ProtoSurfaceMaterial : " << std::endl;
  if (m_binning.has_value()) {
    sl << *m_binning << std::endl;
  } else {
    sl << "no binning (homogeneous)" << std::endl;
  }
  return sl;
}

}  // namespace Acts
