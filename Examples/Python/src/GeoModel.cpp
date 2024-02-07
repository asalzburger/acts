// This file is part of the Acts project.
//
// Copyright (C) 2021 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/GeoModel/GeoModelDetectorElement.hpp"
#include "Acts/Plugins/GeoModel/GeoModelDetectorSurfaceFactory.hpp"
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

  py::class_<Acts::GeoModelDetectorElement,
             std::shared_ptr<Acts::GeoModelDetectorElement>>(
      m, "GeoModelDetectorElement");

  py::class_<Acts::GeoModelTree>(m, "GeoModelTree").def(py::init<>());

  {
    py::module m2 = m.def_submodule("GeoModelReader");
    m2.def("readFromDb", &Acts::GeoModelReader::readFromDb);
  }

  {
    auto f =
        py::class_<Acts::GeoModelDetectorSurfaceFactory,
                   std::shared_ptr<Acts::GeoModelDetectorSurfaceFactory>>(
            m, "GeoModelDetectorSurfaceFactory")
            .def(py::init([](Acts::Logging::Level level) {
              return std::make_shared<Acts::GeoModelDetectorSurfaceFactory>(
                  Acts::getDefaultLogger("GeoModelDetectorSurfaceFactory",
                                         level));
            }))
            .def("construct", &Acts::GeoModelDetectorSurfaceFactory::construct);

    py::class_<Acts::GeoModelDetectorSurfaceFactory::Cache>(f, "Cache")
        .def(py::init<>())
        .def_readwrite(
            "sensitiveSurfaces",
            &Acts::GeoModelDetectorSurfaceFactory::Cache::sensitiveSurfaces);

    py::class_<Acts::GeoModelDetectorSurfaceFactory::Options>(f, "Options")
        .def(py::init<>())
        .def_readwrite("queries",
                       &Acts::GeoModelDetectorSurfaceFactory::Options::queries);
  }
}
}  // namespace Acts::Python
