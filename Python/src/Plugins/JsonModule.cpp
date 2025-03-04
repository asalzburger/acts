// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Detector/Detector.hpp"
#include "Acts/Detector/ProtoDetector.hpp"
#include "Acts/Plugins/Json/DetectorJsonConverter.hpp"
#include "Acts/Plugins/Json/JsonMaterialDecorator.hpp"
#include "Acts/Plugins/Json/JsonSurfacesReader.hpp"
#include "Acts/Plugins/Json/MaterialMapJsonConverter.hpp"
#include "Acts/Plugins/Json/ProtoDetectorJsonConverter.hpp"
#include "Acts/Python/PyUtilities.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <fstream>
#include <initializer_list>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include <nlohmann/json.hpp>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace Acts {
class IMaterialDecorator;
}  // namespace Acts

namespace py = pybind11;
using namespace pybind11::literals;

PYBIND11_MODULE(ActsPythonBindingsJson, json) {

  {
    py::class_<Acts::JsonMaterialDecorator, Acts::IMaterialDecorator,
               std::shared_ptr<Acts::JsonMaterialDecorator>>(json,
                                                       "MaterialDecorator")
        .def(py::init<const Acts::MaterialMapJsonConverter::Config&,
                      const std::string&, Acts::Logging::Level, bool, bool>(),
             py::arg("rConfig"), py::arg("jFileName"), py::arg("level"),
             py::arg("clearSurfaceMaterial") = true,
             py::arg("clearVolumeMaterial") = true);
  }

  {
    auto cls =
        py::class_<Acts::MaterialMapJsonConverter>(json, "MaterialMapConverter")
            .def(py::init<const Acts::MaterialMapJsonConverter::Config&,
                          Acts::Logging::Level>(),
                 py::arg("config"), py::arg("level"));

    auto c = py::class_<Acts::MaterialMapJsonConverter::Config>(cls, "Config")
                 .def(py::init<>());
    ACTS_PYTHON_STRUCT_BEGIN(c, Acts::MaterialMapJsonConverter::Config);
    ACTS_PYTHON_MEMBER(context);
    ACTS_PYTHON_MEMBER(processSensitives);
    ACTS_PYTHON_MEMBER(processApproaches);
    ACTS_PYTHON_MEMBER(processRepresenting);
    ACTS_PYTHON_MEMBER(processBoundaries);
    ACTS_PYTHON_MEMBER(processVolumes);
    ACTS_PYTHON_MEMBER(processDenseVolumes);
    ACTS_PYTHON_MEMBER(processNonMaterial);
    ACTS_PYTHON_STRUCT_END();
  }

  {
    auto sjOptions =
        py::class_<Acts::JsonSurfacesReader::Options>(json, "Surfaceptions")
            .def(py::init<>());

    ACTS_PYTHON_STRUCT_BEGIN(sjOptions, Acts::JsonSurfacesReader::Options);
    ACTS_PYTHON_MEMBER(inputFile);
    ACTS_PYTHON_MEMBER(jsonEntryPath);
    ACTS_PYTHON_STRUCT_END();

    json.def("readSurfaceHierarchyMap",
          Acts::JsonSurfacesReader::readHierarchyMap);

    json.def("readSurfaceVector", Acts::JsonSurfacesReader::readVector);

    py::class_<Acts::JsonDetectorElement, Acts::DetectorElementBase,
               std::shared_ptr<Acts::JsonDetectorElement>>(
        json, "DetectorElement")
        .def("surface", [](Acts::JsonDetectorElement& self) {
          return self.surface().getSharedPtr();
        });

    json.def("readDetectorElements",
          Acts::JsonSurfacesReader::readDetectorElements);
  }

  {
    json.def("writeDetector",
            [](const Acts::GeometryContext& gctx,
               const Acts::Experimental::Detector& detector,
               const std::string& name) -> void {
              auto jDetector =
                  Acts::DetectorJsonConverter::toJson(gctx, detector);
              std::ofstream out;
              out.open(name + ".json");
              out << jDetector.dump(4);
              out.close();
            });
  }

  {
    json.def("writeDetrayDetector",
            [](const Acts::GeometryContext& gctx,
               const Acts::Experimental::Detector& detector,
               const std::string& name) -> void {
              // Detray format test - manipulate for detray
              Acts::DetectorVolumeJsonConverter::Options detrayOptions;
              detrayOptions.transformOptions.writeIdentity = true;
              detrayOptions.transformOptions.transpose = true;
              detrayOptions.surfaceOptions.transformOptions =
                  detrayOptions.transformOptions;
              detrayOptions.portalOptions.surfaceOptions =
                  detrayOptions.surfaceOptions;

              auto jDetector = Acts::DetectorJsonConverter::toJsonDetray(
                  gctx, detector,
                  Acts::DetectorJsonConverter::Options{detrayOptions});

              // Write out the geometry, surface_grid, material
              auto jGeometry = jDetector["geometry"];
              auto jSurfaceGrids = jDetector["surface_grids"];
              auto jMaterial = jDetector["material"];

              std::ofstream out;
              out.open(name + "_geometry_detray.json");
              out << jGeometry.dump(4);
              out.close();

              out.open(name + "_surface_grids_detray.json");
              out << jSurfaceGrids.dump(4);
              out.close();

              out.open(name + "_material_detray.json");
              out << jMaterial.dump(4);
              out.close();
            });
  }

  {
    json.def("readDetector",
            [](const Acts::GeometryContext& gctx,
               const std::string& fileName) -> auto {
              auto in = std::ifstream(
                  fileName, std::ifstream::in | std::ifstream::binary);
              nlohmann::json jDetectorIn;
              in >> jDetectorIn;
              in.close();

              return Acts::DetectorJsonConverter::fromJson(gctx, jDetectorIn);
            });
  }
}
