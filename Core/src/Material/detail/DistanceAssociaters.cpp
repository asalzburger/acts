// This file is part of the Acts project.
//
// Copyright (C) 2024 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "Acts/Material/detail/DistanceAssociaters.hpp"

std::vector<std::tuple<Acts::SurfaceIntersection,
                       std::vector<Acts::MaterialInteraction>>>
Acts::detail::closestOrdered(
    const std::vector<Acts::SurfaceIntersection>& prediction,
    const std::vector<Acts::MaterialInteraction>& materialInteractions) {
  // Prepare the return object
  std::vector<std::tuple<Acts::SurfaceIntersection,
                         std::vector<Acts::MaterialInteraction>>>
      associatedMaterial;

  

  for (const auto& pred : prediction) {
   // Current association
   std::tuple<Acts::SurfaceIntersection,
              std::vector<Acts::MaterialInteraction>>
       currentAssociation = {pred, {};

    for (const auto [im, mInt] : materialInteractions) {
      // Distance to current
      ActsScalar distToCurrent = (pred.position() - mInt.position).mag();
      // Distance to next
      ActsScalar distToNext = 
      
      (pred.position() - mInt.position).mag();


      if (pred.pathLength() > mInt.pathLength) {
        associatedMaterial.push_back(std::make_tuple(
            pred, std::vector<Acts::MaterialInteraction>{mInt}));
        break;
      }
    }
  }

  return associatedMaterial;
}