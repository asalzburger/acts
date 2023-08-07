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
    VolumeMaterialMapper,
    Navigator,
    Propagator,
    StraightLineStepper,
    MaterialMapJsonConverter,
)


def runMaterialMapping(
    trackingGeometry,
    decorators,
    outputDir,
    inputDir,
    mapName="material-map",
    mapSurface=True,
    mapVolume=True,
    readCachedSurfaceInformation=False,
    mappingStep=1,
    s=None,
    ezRange = [ 0., 1000.],
):
    s = s or Sequencer(numThreads=1)

    for decorator in decorators:
        s.addContextDecorator(decorator)

    wb = WhiteBoard(acts.logging.INFO)

    context = AlgorithmContext(0, 0, wb)

    for decorator in decorators:
        assert decorator.decorate(context) == ProcessCode.SUCCESS

    # Read material step information from a ROOT TTRee
    s.addReader(
        RootMaterialTrackReader(
            level=acts.logging.INFO,
            collection="material-tracks",
            fileList=[
                os.path.join(
                    inputDir,
                    mapName + "_tracks.root"
                    if readCachedSurfaceInformation
                    else "geant4_material_tracks.root",
                )
            ],
            readCachedSurfaceInformation=readCachedSurfaceInformation,
        )
    )

    stepper = StraightLineStepper()

    mmAlgCfg = MaterialMapping.Config(context.geoContext, context.magFieldContext)
    mmAlgCfg.trackingGeometry = trackingGeometry
    mmAlgCfg.collection = "material-tracks"

    if mapSurface:
        navigator = Navigator(
            trackingGeometry=trackingGeometry,
            resolveSensitive=True,
            resolveMaterial=True,
            resolvePassive=True,
        )
        propagator = Propagator(stepper, navigator)
        
        mapperCfg = SurfaceMaterialMapper.Config()
        mapperCfg.ezRange = ezRange
        mapper = SurfaceMaterialMapper(config = mapperCfg, level=acts.logging.INFO, propagator=propagator)
        mmAlgCfg.materialSurfaceMapper = mapper

    if mapVolume:
        navigator = Navigator(
            trackingGeometry=trackingGeometry,
        )
        propagator = Propagator(stepper, navigator)
        mapper = VolumeMaterialMapper(
            level=acts.logging.INFO, propagator=propagator, mappingStep=mappingStep
        )
        mmAlgCfg.materialVolumeMapper = mapper

    jmConverterCfg = MaterialMapJsonConverter.Config(
        processSensitives=True,
        processApproaches=True,
        processRepresenting=True,
        processBoundaries=True,
        processVolumes=True,
        context=context.geoContext,
    )

    jmw = JsonMaterialWriter(
        level=acts.logging.VERBOSE,
        converterCfg=jmConverterCfg,
        fileName=os.path.join(outputDir, mapName),
        writeFormat=JsonFormat.Json,
    )

    mmAlgCfg.materialWriters = [jmw]

    s.addAlgorithm(MaterialMapping(level=acts.logging.INFO, config=mmAlgCfg))

    s.addWriter(
        RootMaterialTrackWriter(
            level=acts.logging.INFO,
            collection=mmAlgCfg.mappingMaterialCollection,
            filePath=os.path.join(
                outputDir,
                mapName + "_tracks.root",
            ),
            storeSurface=True,
            storeVolume=True,
        )
    )

    return s


if "__main__" == __name__:

    # Restrict
    elementRange = [ 13.5 , 14.]
    materialMap = "material-map-Z14"

    # Create a single layer tracking geometry, parameters are r, hz, binsZ, binzPhi, material decorator
    trackingGeometry = acts.createSingleCylinderGeometry(34, 400, 100, 36, None)

    runMaterialMapping(
        trackingGeometry,
        decorators = [],
        outputDir=os.getcwd(),
        inputDir=os.getcwd(),
        mapName=materialMap,
        readCachedSurfaceInformation=False,
        ezRange=elementRange,
    ).run()
