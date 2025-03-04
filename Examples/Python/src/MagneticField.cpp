// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/MagneticField/MagneticField.hpp"

#include "Acts/Definitions/Units.hpp"
#include "Acts/MagneticField/BFieldMapUtils.hpp"
#include "Acts/MagneticField/ConstantBField.hpp"
#include "Acts/MagneticField/MagneticFieldProvider.hpp"
#include "Acts/MagneticField/MultiRangeBField.hpp"
#include "Acts/MagneticField/NullBField.hpp"
#include "Acts/MagneticField/SolenoidBField.hpp"
#include "Acts/Python/PyUtilities.hpp"
#include "ActsExamples/MagneticField/FieldMapRootIo.hpp"
#include "ActsExamples/MagneticField/FieldMapTextIo.hpp"

#include <array>
#include <cstddef>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;
using namespace pybind11::literals;

namespace Acts::Python {

/// @brief Get the value of a field, throwing an exception if the result is
/// invalid.
Acts::Vector3 getField(Acts::MagneticFieldProvider& self,
                       const Acts::Vector3& position,
                       Acts::MagneticFieldProvider::Cache& cache) {
  if (Result<Vector3> res = self.getField(position, cache); !res.ok()) {
    std::stringstream ss;

    ss << "Field lookup failure with error: \"" << res.error() << "\"";

    throw std::runtime_error{ss.str()};
  } else {
    return *res;
  }
}

void addMagneticField(Context& ctx) {

  py::class_<ActsExamples::detail::InterpolatedMagneticField2,
             Acts::InterpolatedMagneticField, Acts::MagneticFieldProvider,
             std::shared_ptr<ActsExamples::detail::InterpolatedMagneticField2>>(
      mex, "InterpolatedMagneticField2");

  py::class_<ActsExamples::detail::InterpolatedMagneticField3,
             Acts::InterpolatedMagneticField, Acts::MagneticFieldProvider,
             std::shared_ptr<ActsExamples::detail::InterpolatedMagneticField3>>(
      mex, "InterpolatedMagneticField3");

  py::class_<Acts::NullBField, Acts::MagneticFieldProvider,
             std::shared_ptr<Acts::NullBField>>(m, "NullBField")
      .def(py::init<>());

  py::class_<Acts::MultiRangeBField, Acts::MagneticFieldProvider,
             std::shared_ptr<Acts::MultiRangeBField>>(m, "MultiRangeBField")
      .def(py::init<
           std::vector<std::pair<Acts::RangeXD<3, double>, Acts::Vector3>>>());
  mex.def(
      "MagneticFieldMapXyz",
      [](const std::string& filename, const std::string& tree,
         double lengthUnit, double BFieldUnit, bool firstOctant) {
        const std::filesystem::path file = filename;

        auto mapBins = [](std::array<std::size_t, 3> bins,
                          std::array<std::size_t, 3> sizes) {
          return (bins[0] * (sizes[1] * sizes[2]) + bins[1] * sizes[2] +
                  bins[2]);
        };

        if (file.extension() == ".root") {
          auto map = ActsExamples::makeMagneticFieldMapXyzFromRoot(
              std::move(mapBins), file.native(), tree, lengthUnit, BFieldUnit,
              firstOctant);
          return std::make_shared<
              ActsExamples::detail::InterpolatedMagneticField3>(std::move(map));
        } else if (file.extension() == ".txt") {
          auto map = ActsExamples::makeMagneticFieldMapXyzFromText(
              std::move(mapBins), file.native(), lengthUnit, BFieldUnit,
              firstOctant);
          return std::make_shared<
              ActsExamples::detail::InterpolatedMagneticField3>(std::move(map));
        } else {
          throw std::runtime_error("Unsupported magnetic field map file type");
        }
      },
      py::arg("file"), py::arg("tree") = "bField",
      py::arg("lengthUnit") = Acts::UnitConstants::mm,
      py::arg("BFieldUnit") = Acts::UnitConstants::T,
      py::arg("firstOctant") = false);

  mex.def(
      "MagneticFieldMapRz",
      [](const std::string& filename, const std::string& tree,
         double lengthUnit, double BFieldUnit, bool firstQuadrant) {
        const std::filesystem::path file = filename;

        auto mapBins = [](std::array<std::size_t, 2> bins,
                          std::array<std::size_t, 2> sizes) {
          return (bins[1] * sizes[0] + bins[0]);
        };

        if (file.extension() == ".root") {
          auto map = ActsExamples::makeMagneticFieldMapRzFromRoot(
              std::move(mapBins), file.native(), tree, lengthUnit, BFieldUnit,
              firstQuadrant);
          return std::make_shared<
              ActsExamples::detail::InterpolatedMagneticField2>(std::move(map));
        } else if (file.extension() == ".txt") {
          auto map = ActsExamples::makeMagneticFieldMapRzFromText(
              std::move(mapBins), file.native(), lengthUnit, BFieldUnit,
              firstQuadrant);
          return std::make_shared<
              ActsExamples::detail::InterpolatedMagneticField2>(std::move(map));
        } else {
          throw std::runtime_error("Unsupported magnetic field map file type");
        }
      },
      py::arg("file"), py::arg("tree") = "bField",
      py::arg("lengthUnit") = Acts::UnitConstants::mm,
      py::arg("BFieldUnit") = Acts::UnitConstants::T,
      py::arg("firstQuadrant") = false);
}

}  // namespace Acts::Python
