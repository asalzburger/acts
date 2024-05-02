#!/usr/bin/env python3
import argparse
import json

import acts
from acts import GapVolumeFiller

from acts.examples.dd4hep import (
    DD4hepDetector,
    DD4hepDetectorOptions,
    DD4hepGeometryService,
)
from acts.examples.odd import getOpenDataDetectorDirectory

if "__main__" == __name__:
    p = argparse.ArgumentParser()

    p.add_argument(
        "-m", "--mat-map", type=str, default="", help="Input file for the material map"
    )

    p.add_argument(
        "-g", "--geo-id-map", type=str, default="", help="Geometry id mapping file"
    )

    p.add_argument(
        "--json-detray",
        action=argparse.BooleanOptionalAction,
        help="Output the detector in json format for detray",
    )

    p.add_argument(
        "--obj",
        action=argparse.BooleanOptionalAction,
        help="Output the detector in obj format",
    )

    p.add_argument(
        "--svg",
        action=argparse.BooleanOptionalAction,
        help="Output the detector in svg format",
    )

    p.add_argument(
        "--svg-individuals",
        action=argparse.BooleanOptionalAction,
        help="Output the individual volumes in svg format",
    )

    args = p.parse_args()

    decorators = None
    if len(args.mat_map) > 0:
        decorators = acts.IMaterialDecorator.fromFile(args.mat_map)

    odd_xml = getOpenDataDetectorDirectory() / "xml" / "OpenDataDetector.xml"

    print("Using the following xml file: ", odd_xml)

    # Detector manipulation - add material files
    gvFillerConfig = GapVolumeFiller.Config()
    gvFillerConfig.surfaces = acts.examples.constructMaterialSurfacesODD()

    gvFiller = GapVolumeFiller(gvFillerConfig, "GapVolumeFillerODD", acts.logging.DEBUG)

    # Create the dd4hep geometry service and detector
    dd4hepConfig = DD4hepGeometryService.Config()
    dd4hepConfig.logLevel = acts.logging.INFO
    dd4hepConfig.xmlFileNames = [str(odd_xml)]
    dd4hepGeometryService = DD4hepGeometryService(dd4hepConfig)
    dd4hepDetector = DD4hepDetector(dd4hepGeometryService)

    cOptions = DD4hepDetectorOptions(logLevel=acts.logging.INFO, emulateToGraph="")
    cOptions.materialDecorator = decorators
    cOptions.detectorManipulator = gvFiller

    # Uncomment if you want to use the geometry id mapping
    # This map can be produced with the 'geometry.py' script
    if len(args.geo_id_map) > 0:
        # Load the geometry id mapping json file
        with open(args.geo_id_map) as f:
            # load the file as is
            geometry_id_mapping = json.load(f)
            # create a dictionary with GeometryIdentifier as value
            geometry_id_mapping_patched = {
                int(k): acts.GeometryIdentifier(int(v))
                for k, v in geometry_id_mapping.items()
            }
            # patch the options struct
            acts.examples.dd4hep.attachDD4hepGeoIdMapper(
                cOptions, geometry_id_mapping_patched
            )

    # Context and options
    geoContext = acts.GeometryContext()
    [detector, contextors, store] = dd4hepDetector.finalize(geoContext, cOptions)

    # JSON output
    if args.json_detray:
        acts.examples.writeDetectorToJsonDetray(geoContext, detector, "odd-detray")

    # OBJ style output
    if args.obj:
        surfaces = []
        for vol in detector.volumePtrs():
            for surf in vol.surfacePtrs():
                if surf.geometryId().sensitive() > 0:
                    surfaces.append(surf)
        acts.examples.writeSurfacesObj(
            surfaces, geoContext, [0, 120, 120], "odd-surfaces.obj"
        )

    # SVG style output
    if args.svg:
        surfaceStyle = acts.svg.Style()
        surfaceStyle.fillColor = [5, 150, 245]
        surfaceStyle.fillOpacity = 0.5

        surfaceOptions = acts.svg.SurfaceOptions()
        surfaceOptions.style = surfaceStyle

        viewRange = acts.Extent([])
        volumeOptions = acts.svg.DetectorVolumeOptions()
        volumeOptions.surfaceOptions = surfaceOptions

        if args.svg_individuals:
            for ivol in range(detector.numberVolumes()):
                acts.svg.viewDetector(
                    geoContext,
                    detector,
                    "odd-xy",
                    [[ivol, volumeOptions]],
                    [["xy", ["sensitives"], viewRange]],
                    "vol_" + str(ivol),
                )

        xyRange = acts.Extent([[acts.Binning.z, [-50, 50]]])
        zrRange = acts.Extent([[acts.Binning.phi, [-0.1, 0.1]]])

        acts.svg.viewDetector(
            geoContext,
            detector,
            "odd",
            [[ivol, volumeOptions] for ivol in range(detector.numberVolumes())],
            [["xy", ["sensitives"], xyRange], ["zr", ["materials"], zrRange]],
            "detector",
        )
