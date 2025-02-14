// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/Python/Utilities.hpp"
#include "Acts/Geometry/TrackingGeometry.hpp"
#include "Acts/Plugins/Geant4/Geant4DetectorElement.hpp"
#include "Acts/Plugins/Geant4/Geant4Converters.hpp"
#include "Acts/Plugins/Geant4/Geant4DetectorSurfaceFactory.hpp"
#include "ActsExamples/Geant4/Geant4ConstructionOptions.hpp"
#include "ActsExamples/Geant4/Geant4Manager.hpp"
#include "ActsExamples/Geant4/Geant4Simulation.hpp"
#include "ActsExamples/Geant4/RegionCreator.hpp"
#include "ActsExamples/Geant4/SensitiveSurfaceMapper.hpp"
#include "ActsExamples/Geant4Detector/GdmlDetector.hpp"
#include "ActsExamples/Geant4Detector/GdmlDetectorConstruction.hpp"
#include "Acts/Plugins/Geant4/Geant4PhysicalVolumeSelectors.hpp"

#include <G4Transform3D.hh>

#include <vector>
#include <tuple>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;
using namespace pybind11::literals;

namespace Acts::Python {
void addHiBeam(Context& ctx) {

    auto [m, mex] = ctx.get("main", "examples");

    auto hibeam = mex.def_submodule("hibeam");

    /// This adds a new function to the hibeam submodule
    ///
    /// @param gdmlFile the file for the GDML sources
    /// @param bpVolume the beamPipe Volume name
    /// @param tpcVolume the tbpVolume name
    /// @param tpcSurfaces the number of modelling tpc surfaces
    hibeam.def("buildDetector",[](const std::string& gdmlFile,
                                  const std::string& bpVolume,
                                  const std::string& tpcVolume,
                                  std::size_t tpcSurfaces){

    // Create the detector and get the relevant volumes
    // Initiate the detector construction & retrieve world
    ActsExamples::GdmlDetectorConstruction gdmlContruction(gdmlFile, {});
    const auto* world = gdmlContruction.Construct();

        // Create the selectors
        auto passiveSelectors =
            std::make_shared<Acts::Geant4PhysicalVolumeSelectors::NameSelector>(
                std::vector<std::string>{bpVolume}, false);
        auto sensitiveSelectors =
            std::make_shared<Acts::Geant4PhysicalVolumeSelectors::NameSelector>(
                std::vector<std::string>{tpcVolume}, false);

        Acts::Geant4DetectorSurfaceFactory::Cache cache;
        Acts::Geant4DetectorSurfaceFactory::Options options;
        options.sensitiveSurfaceSelector = sensitiveSelectors;
        options.passiveSurfaceSelector = passiveSelectors;
        options.convertMaterial = false;

        G4Transform3D nominal;
        Acts::Geant4DetectorSurfaceFactory factory;
        factory.construct(cache, nominal, *world, options);

        // The cache should now have (if found correctly 2 entries)
        // cache.
        // 1 - passive surface
        // -> this should be converted into a volume
        // -> cast to cylinder surface
        // -> take length, radius from the cylinder surface bounds
        /// -> make a CylinderVolumeBouns object
        /// -> createa a TrackingVolume ojbect with that bounds   ---> Volume A

        std::vector<std::shared_ptr<Geant4DetectorElement>> dElements = {};

        // 1- sensitive surface
        //
        // -> this should have the one single surface representing the tpc
        // -> take the Geant4DetectorElement that it is associated with
        // -> take the Geant4 object this associated with the detector element
        // -> get Rmin , Rmax
        // -> split into an array of R's from Rmin to Rmax
        // -> create N Acts:CylinderSurface objects
        // -> create for each one a Geant4Detector element:
        //     Geant4DetectorElement(std::shared_ptr<Surface> surface,
        //    const G4VPhysicalVolume& g4physVol,
        //    const Transform3& toGlobal, double thickness);
        //     -> stuff all of these elements into the dElements vector
        //
        // -> make a one CylinderLayer out of every surface
        // -> Use the LayerArrayCreator to make a array of layers
        // -> Make a tracking Volume with Inner Radius = Outer Radius of Volume A
        //    and outer radius bigger than the outermost cylinder surface
        //    -> register the CylidnerLayerArray into this TrackingVolume
        // ------> Volume B

        /// Use the TrackingVolumeArrayCreator to make a volumeArray of Volume A + B
        /// -> Create a container volume that contains A + B

        std::shared_ptr<const TrackingGeometry> tGeometry = nullptr;

        // Stuff that into a newly created TrackingGeometry




        return std::tie(tGeometry, dElements);

    });


}
}
