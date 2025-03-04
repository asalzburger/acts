// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// Must be on top to avoid some conflict between forward declare and typedef
// Needed until https://gitlab.cern.ch/GeoModelDev/GeoModel/-/merge_requests/351
// is deployed
// clang-format off
#include <GeoModelRead/ReadGeoModel.h>
// clang-format on

#include "Acts/Detector/CylindricalContainerBuilder.hpp"
#include "Acts/Plugins/GeoModel/GeoModelBlueprintCreater.hpp"
#include "Acts/Plugins/GeoModel/GeoModelConverters.hpp"
#include "Acts/Plugins/GeoModel/GeoModelDetectorElement.hpp"
#include "Acts/Plugins/GeoModel/GeoModelDetectorElementITk.hpp"
#include "Acts/Plugins/GeoModel/GeoModelDetectorObjectFactory.hpp"
#include "Acts/Plugins/GeoModel/GeoModelReader.hpp"
#include "Acts/Plugins/GeoModel/GeoModelTree.hpp"
#include "Acts/Plugins/GeoModel/IGeoShapeConverter.hpp"
#include "Acts/Python/PyUtilities.hpp"
#include "Acts/Surfaces/AnnulusBounds.hpp"
#include "Acts/Surfaces/DiscSurface.hpp"
#include "Acts/Surfaces/PlaneSurface.hpp"
#include "Acts/Surfaces/RectangleBounds.hpp"

#include <string>

#include <GeoModelKernel/GeoFullPhysVol.h>
#include <GeoModelKernel/GeoVPhysVol.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;
using namespace pybind11::literals;

PYBIND11_MODULE(ActsPythonBindingsGeoModel, geomodel) {
  py::class_<Acts::GeoModelTree>(geomodel, "GeoModelTree").def(py::init<>());

  geomodel.def("readFromDb", &Acts::GeoModelReader::readFromDb);

  py::class_<Acts::GeoModelDetectorElement,
             std::shared_ptr<Acts::GeoModelDetectorElement>>(
      geomodel, "GeoModelDetectorElement")
      .def("logVolName", &Acts::GeoModelDetectorElement::logVolName)
      .def("databaseEntryName",
           &Acts::GeoModelDetectorElement::databaseEntryName)
      .def("surface", [](Acts::GeoModelDetectorElement self) {
        return self.surface().getSharedPtr();
      });

  // Shape converters
  {
    py::class_<Acts::IGeoShapeConverter,
               std::shared_ptr<Acts::IGeoShapeConverter>>(geomodel,
                                                          "IGeoShapeConverter");

    py::class_<Acts::GeoBoxConverter, Acts::IGeoShapeConverter,
               std::shared_ptr<Acts::GeoBoxConverter>>(geomodel,
                                                       "GeoBoxConverter")
        .def(py::init<>())
        .def("toSensitiveSurface", &Acts::GeoBoxConverter::toSensitiveSurface)
        .def("toPassiveSurface", &Acts::GeoBoxConverter::toPassiveSurface);

    py::class_<Acts::GeoTrdConverter, Acts::IGeoShapeConverter,
               std::shared_ptr<Acts::GeoTrdConverter>>(geomodel,
                                                       "GeoTrdConverter")
        .def(py::init<>())
        .def("toSensitiveSurface", &Acts::GeoTrdConverter::toSensitiveSurface)
        .def("toPassiveSurface", &Acts::GeoTrdConverter::toPassiveSurface);

    py::class_<Acts::GeoTubeConverter, Acts::IGeoShapeConverter,
               std::shared_ptr<Acts::GeoTubeConverter>>(geomodel,
                                                        "GeoTubeConverter")
        .def(py::init<>())
        .def("toSensitiveSurface", &Acts::GeoTubeConverter::toSensitiveSurface)
        .def("toPassiveSurface", &Acts::GeoTubeConverter::toPassiveSurface);

    py::class_<Acts::GeoUnionDoubleTrdConverter, Acts::IGeoShapeConverter,
               std::shared_ptr<Acts::GeoUnionDoubleTrdConverter>>(
        geomodel, "GeoUnionDoubleTrdConverter")
        .def(py::init<>())
        .def("toSensitiveSurface",
             &Acts::GeoUnionDoubleTrdConverter::toSensitiveSurface)
        .def("toPassiveSurface",
             &Acts::GeoUnionDoubleTrdConverter::toPassiveSurface);

    py::class_<Acts::GeoIntersectionAnnulusConverter, Acts::IGeoShapeConverter,
               std::shared_ptr<Acts::GeoIntersectionAnnulusConverter>>(
        geomodel, "GeoIntersectionAnnulusConverter")
        .def(py::init<>())
        .def("toSensitiveSurface",
             &Acts::GeoIntersectionAnnulusConverter::toSensitiveSurface)
        .def("toPassiveSurface",
             &Acts::GeoIntersectionAnnulusConverter::toPassiveSurface);

    py::class_<Acts::GeoShiftConverter, Acts::IGeoShapeConverter,
               std::shared_ptr<Acts::GeoShiftConverter>>(geomodel,
                                                         "GeoShiftConverter")
        .def(py::init<>())
        .def("toSensitiveSurface", &Acts::GeoShiftConverter::toSensitiveSurface)
        .def("toPassiveSurface", &Acts::GeoShiftConverter::toPassiveSurface);
  }

  // Volume factory
  {
    auto a =
        py::class_<Acts::GeoModelDetectorObjectFactory,
                   std::shared_ptr<Acts::GeoModelDetectorObjectFactory>>(
            geomodel, "GeoModelDetectorObjectFactory")
            .def(py::init(
                [](const Acts::GeoModelDetectorObjectFactory::Config& cfg,
                   Acts::Logging::Level level) {
                  return std::make_shared<Acts::GeoModelDetectorObjectFactory>(
                      cfg, Acts::getDefaultLogger(
                               "GeoModelDetectorObjectFactory", level));
                }))
            .def("construct", &Acts::GeoModelDetectorObjectFactory::construct);

    py::class_<Acts::GeoModelDetectorObjectFactory::Config>(a, "Config")
        .def(py::init<>())
        .def_readwrite(
            "convertSubVolumes",
            &Acts::GeoModelDetectorObjectFactory::Config::convertSubVolumes)
        .def_readwrite("nameList",
                       &Acts::GeoModelDetectorObjectFactory::Config::nameList)
        .def_readwrite("convertBox",
                       &Acts::GeoModelDetectorObjectFactory::Config::convertBox)
        .def_readwrite(
            "materialList",
            &Acts::GeoModelDetectorObjectFactory::Config::materialList);

    py::class_<Acts::GeoModelDetectorObjectFactory::Cache>(a, "Cache")
        .def(py::init<>())
        .def_readwrite(
            "sensitiveSurfaces",
            &Acts::GeoModelDetectorObjectFactory::Cache::sensitiveSurfaces)
        .def_readwrite(
            "boundingBoxes",
            &Acts::GeoModelDetectorObjectFactory::Cache::boundingBoxes);

    py::class_<Acts::GeoModelDetectorObjectFactory::Options>(a, "Options")
        .def(py::init<>())
        .def_readwrite("queries",
                       &Acts::GeoModelDetectorObjectFactory::Options::queries);
  }

  {
    py::class_<Acts::GeoModelBlueprintCreater::Blueprint,
               std::shared_ptr<Acts::GeoModelBlueprintCreater::Blueprint>>(
        geomodel, "Blueprint")
        .def("convertToBuilder",
             [](Acts::GeoModelBlueprintCreater::Blueprint& self,
                Acts::Logging::Level level) {
               // It's a container builder
               return std::make_shared<
                   Acts::Experimental::CylindricalContainerBuilder>(self.node(),
                                                                    level);
             });

    auto bpc =
        py::class_<Acts::GeoModelBlueprintCreater,
                   std::shared_ptr<Acts::GeoModelBlueprintCreater>>(
            geomodel, "GeoModelBlueprintCreater")
            .def(py::init([](const Acts::GeoModelBlueprintCreater::Config& cfg,
                             Acts::Logging::Level level) {
              return std::make_shared<Acts::GeoModelBlueprintCreater>(
                  cfg,
                  Acts::getDefaultLogger("GeoModelBlueprintCreater", level));
            }))
            .def("create", &Acts::GeoModelBlueprintCreater::create);

    py::class_<Acts::GeoModelBlueprintCreater::Config>(bpc, "Config")
        .def(py::init<>())
        .def_readwrite(
            "detectorSurfaces",
            &Acts::GeoModelBlueprintCreater::Config::detectorSurfaces)
        .def_readwrite("kdtBinning",
                       &Acts::GeoModelBlueprintCreater::Config::kdtBinning);

    py::class_<Acts::GeoModelBlueprintCreater::Options>(bpc, "Options")
        .def(py::init<>())
        .def_readwrite("topEntry",
                       &Acts::GeoModelBlueprintCreater::Options::topEntry)
        .def_readwrite(
            "topBoundsOverride",
            &Acts::GeoModelBlueprintCreater::Options::topBoundsOverride)
        .def_readwrite("table", &Acts::GeoModelBlueprintCreater::Options::table)
        .def_readwrite("dotGraph",
                       &Acts::GeoModelBlueprintCreater::Options::dotGraph);
  }

  py::class_<Acts::GeoModelDetectorElementITk,
             std::shared_ptr<Acts::GeoModelDetectorElementITk>>(
      geomodel, "GeoModelDetectorElementITk")
      .def("surface", [](Acts::GeoModelDetectorElementITk& self) {
        return self.surface().getSharedPtr();
      });

  geomodel.def("convertToItk",
               &Acts::GeoModelDetectorElementITk::convertFromGeomodel);
}
