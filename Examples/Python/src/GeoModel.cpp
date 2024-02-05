// This file is part of the Acts project.
//
// Copyright (C) 2021 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/GeoModel/GeoModelReader.hpp"
#include "Acts/Plugins/GeoModel/GeoModelTree.hpp"
#include "Acts/Plugins/Python/Utilities.hpp"

#include <string>

#include <GeoModelKernel/GeoFullPhysVol.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;
using namespace pybind11::literals;

namespace Acts::Python {
void addGeoModel(Context& ctx) {
  auto m = ctx.get("main");

  py::class_<Acts::GeoModelTree>(m, "GeoModelTree").def(py::init<>());

  {
    py::module m2 = m.def_submodule("GeoModelReader");
    m2.def("readFromDb", &Acts::GeoModelReader::readFromDb);
  }
}
}  // namespace Acts::Python
