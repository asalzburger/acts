// This file is part of the Acts project.
//
// Copyright (C) 2021-2022 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/Python/Utilities.hpp"
#include "ActsExamples/Tutorial/UserAlgorithm.hpp"

#include <memory>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

using namespace ActsExamples;
using namespace Acts;

namespace Acts::Python {

void addTutorial(Context& ctx) {
  auto mex = ctx.get("examples");

  {
    using Config = ActsExamples::UserAlgorithm::Config;

    auto alg =
        py::class_<ActsExamples::UserAlgorithm, ActsExamples::BareAlgorithm,
                   std::shared_ptr<ActsExamples::UserAlgorithm>>(
            mex, "UserAlgorithm")
            .def(py::init<const Config&, Acts::Logging::Level>(),
                 py::arg("config"), py::arg("level"))
            .def_property_readonly("config",
                                   &ActsExamples::UserAlgorithm::config);

    auto c = py::class_<Config>(alg, "Config").def(py::init<>());
    ACTS_PYTHON_STRUCT_BEGIN(c, Config);
    ACTS_PYTHON_MEMBER(message);
    ACTS_PYTHON_MEMBER(inputStepCollection);
    ACTS_PYTHON_STRUCT_END();
  }

}

}  // namespace Acts::Python
