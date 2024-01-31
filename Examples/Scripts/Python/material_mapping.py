#!/usr/bin/env python3
import os

from acts.examples import (
    Sequencer,
    WhiteBoard,
    AlgorithmContext,
    ProcessCode,
    RootMaterialTrackReader,
    RootMaterialTrackWriter,
    MaterialMapping,
    JsonMaterialWriter,
    JsonFormat,
)

import acts
from acts import (
    Vector4,
    UnitConstants as u,
    SurfaceMaterialMapper,
    MaterialMapJsonConverter,
)
from acts.examples.dd4hep import (
    DD4hepDetector,
    DD4hepDetectorOptions,
    DD4hepGeometryService,
)
from common import getOpenDataDetectorDirectory
from acts.examples.odd import getOpenDataDetector


def runMaterialMapping(
    detector,
    outputDir,
    inputDir,
    mapName="material-map",
):
    s = Sequencer(numThreads=1)
    wb = WhiteBoard(acts.logging.INFO)

    context = AlgorithmContext(0, 0, wb)

    # Read material step information from a ROOT TTRee
    s.addReader(
        RootMaterialTrackReader(
            level=acts.logging.INFO,
            collection="material-tracks",
            fileList=[
                os.path.join(
                    inputDir,
                    "geant4_material_tracks.root",
                )
            ],
            readCachedSurfaceInformation=False,
        )
    )
    materialSurfaces = acts.examples.extractMaterialSurfaces(detector)
    print(len(materialSurfaces))

    # Make a material mapper
    smmConfig = SurfaceMaterialMapper.Config(surfaces=materialSurfaces)
    smm = SurfaceMaterialMapper(config=smmConfig, level=acts.logging.INFO)

    # Make 
    mmAlgCfg = MaterialMapping.Config()
    mmAlgCfg.materialMapper = smm
    mmAlgCfg.collection = "material-tracks"
    s.addAlgorithm(MaterialMapping(level=acts.logging.INFO, config=mmAlgCfg))

    return s


if "__main__" == __name__:
    odd_xml = getOpenDataDetectorDirectory() / "xml" / "OpenDataDetector.xml"

    print("Using the following xml file: ", odd_xml)

    # Create the dd4hep geometry service and detector
    dd4hepConfig = DD4hepGeometryService.Config()
    dd4hepConfig.logLevel = acts.logging.INFO
    dd4hepConfig.xmlFileNames = [str(odd_xml)]
    dd4hepGeometryService = DD4hepGeometryService(dd4hepConfig)
    dd4hepDetector = DD4hepDetector(dd4hepGeometryService)

    cOptions = DD4hepDetectorOptions(logLevel=acts.logging.INFO, emulateToGraph="")

    # Context and options
    geoContext = acts.GeometryContext()
    [detector, contextors, store] = dd4hepDetector.finalize(geoContext, cOptions)

    runMaterialMapping(
        detector,
        outputDir=os.getcwd(),
        inputDir=os.getcwd(),
    ).run()
