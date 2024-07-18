# import the necessary modules

import acts, acts.examples
from acts.examples import geant4 as acts_g4
import sys, argparse


from acts import (
    #   svg,
    logging,
    Binning,
    Extent,
    GeometryContext,
    ProtoBinning,
    LayerStructureBuilder,
    VolumeBoundsType,
    VolumeStructureBuilder,
    DetectorVolumeBuilder,
    GeometryIdGenerator,
    CuboidalContainerBuilder,
    DetectorBuilder,
    Transform3,
    RangeXDDim1,
    KdtSurfacesDim1Bin100,
    KdtSurfacesProviderDim1Bin100,
)

geoContext = acts.GeometryContext()


def create_na60plus(
    filename,
    sensitive_matches=["PixSensor"],
    passive_matches=["PixStnFrame"],
    logLevel=logging.VERBOSE,
):

    # Load the GDML file and get the surfacessensitive_matches = ["PixSensor"]
    [elements, ssurfaces, psurfaces] = acts_g4.convertSurfaces(
        "na60VT.gdml", sensitive_matches, passive_matches
    )

    # Write them to an obj file
    drawContext = acts.GeometryContext()
    sensitiveRgb = [0, 150, 150]
    passiveRgb = [150, 150, 0]

    kdtSurfaces = KdtSurfacesDim1Bin100(geoContext, ssurfaces, [Binning.z])

    layers = [
        [-25, 65],
        [65, 75],
        [75, 145],
        [145, 155],
        [155, 195],
        [195, 205],
        [205, 246],
        [246, 256],
        [256, 376],
        [376, 386],
    ]

    volBuilders = []
    gaps = 0
    layerId = 0

    for il, lrange in enumerate(layers):
        surfaces = kdtSurfaces.surfaces(RangeXDDim1(lrange))
        layerStr = "Layer_" + str(layerId)
        if len(surfaces) == 0:
            layerStr = "Gap_" + str(gaps)
            gaps += 1
        else:
            layerId += 1

        print("-> Building Volume: ", layerStr, " with surfaces: ", len(surfaces))

        shapeConfig = VolumeStructureBuilder.Config()
        shapeConfig.boundsType = VolumeBoundsType.Cuboid
        shapeConfig.boundValues = [350.0, 350.0, 0.5 * (lrange[1] - lrange[0])]
        shapeConfig.transform = Transform3([0, 0, 0.5 * (lrange[1] + lrange[0])])
        shapeConfig.auxiliary = "Shape[" + layerStr + "]"

        layerStructure = None

        if len(surfaces) > 0:
            surfaceProvider = KdtSurfacesProviderDim1Bin100(
                kdtSurfaces, Extent([[Binning.z, lrange]])
            )

            layerConfig = LayerStructureBuilder.Config()
            layerConfig.surfacesProvider = surfaceProvider
            layerConfig.binnings = []
            layerConfig.supports = []
            layerConfig.auxiliary = layerStr

            layerStructure = LayerStructureBuilder(
                layerConfig, layerConfig.auxiliary, logLevel
            )

        volConfig = acts.DetectorVolumeBuilder.Config()
        volConfig.name = "Volume[" + layerStr + "]"
        volConfig.auxiliary = volConfig.name
        volConfig.externalsBuilder = VolumeStructureBuilder(
            shapeConfig, shapeConfig.auxiliary, logLevel
        )
        volConfig.internalsBuilder = layerStructure

        matXBinning = acts.ProtoBinning(acts.Binning.x, acts.Binning.bound,20,0)
        matYBinning = acts.ProtoBinning(acts.Binning.y, acts.Binning.bound,20,0)
        matBinning = acts.BinningDescription([matXBinning, matYBinning])
        volConfig.portalMaterialBinning[0] = matBinning

        volBuilder = DetectorVolumeBuilder(volConfig, volConfig.name, logLevel)
        volBuilders += [volBuilder]

    print("-> Building Detector from ", len(volBuilders), " volumes")

    geoIdConfig = GeometryIdGenerator.Config()
    geoIdConfig.containerMode = True
    geoId = GeometryIdGenerator(geoIdConfig, "GeometryIdGenerator", logLevel)

    ccConfig = CuboidalContainerBuilder.Config()
    ccConfig.builders = volBuilders
    ccConfig.binning = Binning.z
    ccConfig.geoIdGenerator = geoId
    ccConfig.auxiliary = "SiliconTracker"

    ccBuilder = CuboidalContainerBuilder(ccConfig, ccConfig.auxiliary, logLevel)

    detConfig = DetectorBuilder.Config()
    detConfig.name = "na60+"
    detConfig.builder = ccBuilder
    detConfig.auxiliary = detConfig.name
    detConfig.geoIdGenerator = geoId

    detBuilder = DetectorBuilder(detConfig, detConfig.name, logLevel)
    detector = detBuilder.construct(geoContext)
    return elements, detector


if __name__ == "__main__":
    p = argparse.ArgumentParser()

    p.add_argument(
        "-g", "--gdml", type=str, default="", help="Input GDML file"
    )

    p.add_argument(
        "--output-svg",
        action=argparse.BooleanOptionalAction,
        help="Output the detector in svg",
    )

    p.add_argument(
        "--output-obj",
        action=argparse.BooleanOptionalAction,
        help="Output the detector in obj",
    )

    args = p.parse_args()

    elements, detector = create_na60plus("na60VT.gdml")

    # OBJ style output
    if args.output_obj:
        surfaces = []
        for vol in detector.volumePtrs():
            for surf in vol.surfacePtrs():
                surfaces.append(surf)
        acts.examples.writeSurfacesObj(
            surfaces, geoContext, [0, 120, 120], 3, "nap60-surfaces.obj"
        )

    if args.output_svg:
        # SVG style output
        surfaceStyle = acts.svg.Style()
        surfaceStyle.fillColor = [5, 150, 245]
        surfaceStyle.fillOpacity = 0.5

        surfaceOptions = acts.svg.SurfaceOptions()
        surfaceOptions.style = surfaceStyle

        viewRange = acts.Extent([])
        volumeOptions = acts.svg.DetectorVolumeOptions()
        volumeOptions.surfaceOptions = surfaceOptions

        xyRange = acts.Extent([[acts.Binning.z, [-50, 50]]])
        zrRange = acts.Extent([[acts.Binning.phi, [-0.1, 0.1]]])

        acts.svg.viewDetector(
            geoContext,
            detector,
            "odd",
            [[ivol, volumeOptions] for ivol in range(detector.numberVolumes())],
            [["xy", ["sensitives"], xyRange], ["zr", ["materials"], zrRange]],
            "na60plus_detector",
        )
