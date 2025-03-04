// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/ActsVersion.hpp"
#include "Acts/Python/PyUtilities.hpp"

#include <tuple>
#include <unordered_map>

#include <pybind11/detail/common.h>
#include <pybind11/functional.h>
#include <pybind11/operators.h>
#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>
#include <pybind11/stl.h>
#include <pyerrors.h>

namespace py = pybind11;

namespace Acts::Python {
void addCoreEntry(Context& ctx);
void addPluginsEntry(Context& ctx);
}

PYBIND11_MODULE(ActsPythonBindings, m) {
  Acts::Python::Context ctx;
  ctx.modules["main"] = m;
  m.doc() = "Acts";

  m.attr("__version__") =
      std::tuple{Acts::VersionMajor, Acts::VersionMinor, Acts::VersionPatch};

  {
    auto mv = m.def_submodule("version");

    mv.attr("major") = Acts::VersionMajor;
    mv.attr("minor") = Acts::VersionMinor;
    mv.attr("patch") = Acts::VersionPatch;

    mv.attr("commit_hash") = Acts::CommitHash;
    mv.attr("commit_hash_short") = Acts::CommitHashShort;
  }

  addCoreEntry(ctx);
  addPluginsEntry(ctx);

}
