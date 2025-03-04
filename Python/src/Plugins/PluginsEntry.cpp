// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Python/PyUtilities.hpp"

namespace Acts::Python {

void addCovfie(Context& ctx);
void addDetray(Context& ctx);
void addJson(Context& ctx);
void addSvg(Context& ctx);

// Define plugins entry
void addPluginsEntry(Context& ctx);

}  // namespace Acts::Python

void Acts::Python::addPluginsEntry(Context& ctx) {
  addCovfie(ctx);
  addDetray(ctx);
  addJson(ctx);
  addSvg(ctx);
}
