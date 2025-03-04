// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Python/PyUtilities.hpp"

namespace Acts::Python {
void addContext(Context& ctx);
void addAny(Context& ctx);
void addUnits(Context& ctx);
void addLogging(Context& ctx);
void addPdgParticle(Context& ctx);
void addAlgebra(Context& ctx);
void addBinning(Context& ctx);
void addGeometry(Context& ctx);
void addGeometryBuildingGen1(Context& ctx);
void addNavigation(Context& ctx);
void addExperimentalGeometry(Context& ctx);
void addMaterial(Context& ctx);
void addVisualization(Context& ctx);

// Define core entry
void addCoreEntry(Context& ctx);

}  // namespace Acts::Python

void Acts::Python::addCoreEntry(Context& ctx) {

  addContext(ctx);
  addAny(ctx);
  addUnits(ctx);
  addLogging(ctx);
  addPdgParticle(ctx);
  addAlgebra(ctx);
  addBinning(ctx);
  addVisualization(ctx);
  addGeometry(ctx);
  addGeometryBuildingGen1(ctx);
  addNavigation(ctx);
  addExperimentalGeometry(ctx);
  addMaterial(ctx);

}
