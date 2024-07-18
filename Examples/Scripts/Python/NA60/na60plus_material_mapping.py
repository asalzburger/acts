#!/usr/bin/env python3

import argparse

import acts
import na60plus_detector

from acts.examples import (
    Sequencer,
    WhiteBoard,
    AlgorithmContext,
    RootMaterialTrackReader,
    RootMaterialTrackWriter,
    CoreMaterialMapping,
    JsonMaterialWriter,
    RootMaterialWriter,
    JsonFormat,
)

def runMaterialMapping(surfaces, inputFile, outputFile, outputMap, loglevel):
    # Create a sequencer
    print("Creating the sequencer with 1 thread (inter event information needed)")

    s = Sequencer(numThreads=1)

    # IO for material tracks reading
    wb = WhiteBoard(acts.logging.INFO)

    # Read material step information from a ROOT TTRee
    s.addReader(
        RootMaterialTrackReader(
            level=acts.logging.INFO,
            outputMaterialTracks="material-tracks",
            fileList=[inputFile],
            readCachedSurfaceInformation=False,
        )
    )

    # Assignment setup : Intersection assigner
    materialAssingerConfig = acts.IntersectionMaterialAssigner.Config()
    materialAssingerConfig.surfaces = surfaces
    materialAssinger = acts.IntersectionMaterialAssigner(materialAssingerConfig, loglevel)

    # Accumulation setup : Binned surface material accumulater
    materialAccumulaterConfig = acts.BinnedSurfaceMaterialAccumulater.Config()
    materialAccumulaterConfig.materialSurfaces = surfaces
    materialAccumulater = acts.BinnedSurfaceMaterialAccumulater(
        materialAccumulaterConfig, loglevel
    )

    # Mapper setup
    materialMapperConfig = acts.MaterialMapper.Config()
    materialMapperConfig.assignmentFinder = materialAssinger
    materialMapperConfig.surfaceMaterialAccumulater = materialAccumulater
    materialMapper = acts.MaterialMapper(materialMapperConfig, loglevel)

    # Add the map writer(s)
    mapWriters = []
    # json map writer
    context = AlgorithmContext(0, 0, wb)
    jmConverterCfg = acts.MaterialMapJsonConverter.Config(
        processSensitives=True,
        processApproaches=True,
        processRepresenting=True,
        processBoundaries=True,
        processVolumes=True,
        context=context.geoContext,
    )
    mapWriters.append(
        JsonMaterialWriter(
            level=loglevel,
            converterCfg=jmConverterCfg,
            fileName=outputMap + "",
            writeFormat=JsonFormat.Json,
        )
    )
    mapWriters.append(RootMaterialWriter(level=loglevel, filePath=outputMap + ".root"))

    # Mapping Algorithm
    coreMaterialMappingConfig = CoreMaterialMapping.Config()
    coreMaterialMappingConfig.materialMapper = materialMapper
    coreMaterialMappingConfig.inputMaterialTracks = "material-tracks"
    coreMaterialMappingConfig.mappedMaterialTracks = "mapped-material-tracks"
    coreMaterialMappingConfig.unmappedMaterialTracks = "unmapped-material-tracks"
    coreMaterialMappingConfig.materiaMaplWriters = mapWriters
    coreMaterialMapping = CoreMaterialMapping(coreMaterialMappingConfig, loglevel)
    s.addAlgorithm(coreMaterialMapping)

    # Add the mapped material tracks writer
    s.addWriter(
        RootMaterialTrackWriter(
            level=acts.logging.INFO,
            inputMaterialTracks="mapped-material-tracks",
            filePath=outputFile + "_mapped.root",
            storeSurface=True,
            storeVolume=True,
        )
    )

    # Add the unmapped material tracks writer
    s.addWriter(
        RootMaterialTrackWriter(
            level=acts.logging.INFO,
            inputMaterialTracks="unmapped-material-tracks",
            filePath=outputFile + "_unmapped.root",
            storeSurface=True,
            storeVolume=True,
        )
    )

    return s


if "__main__" == __name__:
    p = argparse.ArgumentParser()

    p.add_argument(
        "-n", "--events", type=int, default=1000, help="Number of events to process"
    )
    p.add_argument(
        "-i", "--input", type=str, default="", help="Input file with material tracks"
    )
    p.add_argument(
        "-g", "--gdml", type=str, default="", help="Input GDML file"
    )
    p.add_argument(
        "-o", "--output", type=str, default="", help="Output file (core) name"
    )
    p.add_argument(
        "-m", "--map", type=str, default="", help="Output file for the material map"
    )

    args = p.parse_args()
    gContext = acts.GeometryContext()
    logLevel = acts.logging.INFO


    elements, detector = na60plus_detector.create_na60plus(args.input)

    materialSurfaces = detector.extractMaterialSurfaces()

    runMaterialMapping(
        materialSurfaces, args.input, args.output, args.map, logLevel
    ).run()
