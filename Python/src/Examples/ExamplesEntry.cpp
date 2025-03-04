// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Python/PyUtilities.hpp"

namespace Acts::Python {
void addFramework(Context& ctx);
// Define core entry
void addExamplesEntry(Context& ctx);

}  // namespace Acts::Python

void Acts::Python::addExamplesEntry(Context& ctx) {
  addFramework(ctx);
}