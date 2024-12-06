#!/usr/bin/env python3

import os
import json

import acts
from acts import MaterialMapJsonConverter
from acts.examples.odd import getOpenDataDetector
from acts.examples import (
    GenericDetector,
    AlignedDetector,
    WhiteBoard,
    AlgorithmContext,
    ProcessCode,
    CsvTrackingGeometryWriter,
    ObjTrackingGeometryWriter,
    JsonSurfacesWriter,
    JsonMaterialWriter,
    JsonFormat,
)


def runGeometry(
    trackingGeometry,
    decorators,
    outputDir,
    events=1,
    outputObj=True,
    outputCsv=True,
    outputJson=True,
    outputSvg=True,
):
    for ievt in range(events):
        eventStore = WhiteBoard(name=f"EventStore#{ievt}", level=acts.logging.INFO)
        ialg = 0

        context = AlgorithmContext(ialg, ievt, eventStore)

        for cdr in decorators:
            r = cdr.decorate(context)
            if r != ProcessCode.SUCCESS:
                raise RuntimeError("Failed to decorate event context")

        if outputCsv:
            # if not os.path.isdir(outputDir + "/csv"):
            #    os.makedirs(outputDir + "/csv")
            writer = CsvTrackingGeometryWriter(
                level=acts.logging.INFO,
                trackingGeometry=trackingGeometry,
                outputDir=os.path.join(outputDir, "csv"),
                writePerEvent=True,
            )
            writer.write(context)

        if outputObj:
            writer = ObjTrackingGeometryWriter(
                level=acts.logging.INFO, outputDir=os.path.join(outputDir, "obj")
            )
            writer.write(context, trackingGeometry)

        if outputJson:
            # if not os.path.isdir(outputDir + "/json"):
            #    os.makedirs(outputDir + "/json")
            writer = JsonSurfacesWriter(
                level=acts.logging.INFO,
                trackingGeometry=trackingGeometry,
                outputDir=os.path.join(outputDir, "json"),
                writePerEvent=True,
                writeSensitive=True,
            )
            writer.write(context)

            jmConverterCfg = MaterialMapJsonConverter.Config(
                processSensitives=True,
                processApproaches=True,
                processRepresenting=True,
                processBoundaries=True,
                processVolumes=True,
                processNonMaterial=True,
                context=context.geoContext,
            )

            jmw = JsonMaterialWriter(
                level=acts.logging.VERBOSE,
                converterCfg=jmConverterCfg,
                fileName=os.path.join(outputDir, "geometry-map"),
                writeFormat=JsonFormat.Json,
            )

            jmw.write(trackingGeometry)

        if outputSvg:
            config = acts.examples.SvgEventWriter.Config()
            config.views = ["xy", "zr"]
            config.sensitiveSurfaces = trackingGeometry.geoIdSurfaceMap()

            xyRange = acts.Extent()
            xyRange.setRange(acts.BinningValue.binZ, -100, 100)
            zrRange = acts.Extent()
            zrRange.setRange(acts.BinningValue.binPhi, -0.2, 0.2)
            sensitiveViewRange = { "xy": [xyRange], "zr": [zrRange] }

            config.sensitiveViewRange = sensitiveViewRange

            # if not os.path.isdir(outputDir + "/svg"):
            #    os.makedirs(outputDir + "/svg")
            writer = acts.examples.SvgEventWriter(
                config=config,
                level=acts.logging.INFO,
            )
            writer.write(context)


if "__main__" == __name__:
    # detector, trackingGeometry, decorators = AlignedDetector.create()
    # detector, trackingGeometry, decorators = GenericDetector.create()
    detector, trackingGeometry, decorators = getOpenDataDetector()

    runGeometry(trackingGeometry, decorators, outputDir=os.getcwd())

    # Uncomment if you want to create the geometry id mapping for DD4hep
    # dd4hepIdGeoIdMap = acts.examples.dd4hep.createDD4hepIdGeoIdMap(trackingGeometry)
    # dd4hepIdGeoIdValueMap = {}
    # for key, value in dd4hepIdGeoIdMap.items():
    #     dd4hepIdGeoIdValueMap[key] = value.value()

    # with open('odd-dd4hep-geoid-mapping.json', 'w') as outfile:
    #    json.dump(dd4hepIdGeoIdValueMap, outfile)
