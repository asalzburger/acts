import acts
import math
from acts import examples
import collections


leafDescription = collections.namedtuple("LeafDescription", [
    "name", "search_range", "bins_l0", "bins_l1",  "extent", "binning", "surfaces"])

""" Extent to cyldinder bounds """


def extent_to_cylinder_parameters(extent: acts.Extent) -> tuple[list[float], acts.Transform3]:
    # Get min max and calculate the values
    rRange = extent.range(acts.BinningValue.binR)
    zRange = extent.range(acts.BinningValue.binZ)

    rMin = rRange[0]
    rMax = rRange[1]
    zMin = zRange[0]
    zMax = zRange[1]
    halfZ = (zMax - zMin) / 2
    zCenter = (zMin + zMax) / 2

    return ([rMin, rMax, halfZ, math.pi, 0.], acts.Transform3(acts.Vector3(0., 0., zCenter)))


""" This method completes the leaf structure of the barrel """


def complete_leaf_description(geoContext: acts.GeometryContext,
                                     leafDescriptions: list[leafDescription],
                                     rzKdtSurfaces: acts.KdtSurfacesDim2Bin100,
                                     envelope_r: float,
                                     envelope_z: float,
                                     barrel : bool = True) -> list[leafDescription]:
    # Complete the leafs for the barrel
    completeLeafDescriptions = []
    for lDescription in leafDescriptions:
        # grab the surfaces and measure them
        lDescriptionSurfaces = rzKdtSurfaces.surfaces(
            lDescription.search_range)
        lDescriptionExtent = acts.Extent([(acts.BinningValue.binZ, (100000., -100000.)),
                                          (acts.BinningValue.binR, (100000., 0))])
        lDescriptionExtent.extendToSurfaces(
            geoContext, lDescriptionSurfaces, 1)
        lDescriptionRangeR = lDescriptionExtent.range(acts.BinningValue.binR)
        lDescriptionRangeZ = lDescriptionExtent.range(acts.BinningValue.binZ)
        lDescriptionRangeR[0] = math.floor(
            lDescriptionRangeR[0] - envelope_r)
        lDescriptionRangeR[1] = math.ceil(
            lDescriptionRangeR[1] - envelope_r)
        lDescriptionRangeZ[0] = math.floor(
            lDescriptionRangeZ[0] - envelope_z)
        lDescriptionRangeZ[1] = math.ceil(
            lDescriptionRangeZ[1] - envelope_z)
        lDescriptionExtent = acts.Extent([(acts.BinningValue.binZ, lDescriptionRangeZ),
                                          (acts.BinningValue.binR, lDescriptionRangeR)])

        # Create the binning
        surfaceBinningValueL0 = acts.BinningValue.binZ if barrel else acts.BinningValue.binR
        surfaceBinningRZ = acts.ProtoBinning(surfaceBinningValueL0, acts.AxisBoundaryType.bound,
                                            lDescriptionRangeZ[0], lDescriptionRangeZ[1], lDescription.bins_l0, 1)
        surfaceBinningPhi = acts.ProtoBinning(acts.BinningValue.binPhi, acts.AxisBoundaryType.closed,
                                              -math.pi, math.pi, lDescription.bins_l1, 1)

        lDescription = lDescription._replace(extent=lDescriptionExtent)
        lDescription = lDescription._replace(surfaces=lDescriptionSurfaces)
        lDescription = lDescription._replace(
            binning=[surfaceBinningRZ, surfaceBinningPhi])

        print(">> Leaf:", lDescription.name,
              "surfaces:", len(lDescriptionSurfaces),
              "r-range:", lDescription.extent.range(acts.BinningValue.binR),
              "z-range:", lDescription.extent.range(acts.BinningValue.binZ))

        completeLeafDescriptions.append(lDescription)

    return completeLeafDescriptions


""" Create the barrel leafs and fill into the geometry """


def create_leaf_nodes(geoContext: acts.GeometryContext,
                      leafDescriptions: list[leafDescription],
                      geoIdGenerator=None,
                      containerExtent=None,
                      barrel=True,
                      symmetrize=False) -> list[acts.BlueprintNode]:
    # Create the leafs
    leafNodes = []
    if containerExtent is None:
        containerExtent = acts.Extent([(acts.BinningValue.binZ, (100000, -100000)),
                                       (acts.BinningValue.binR, (100000., 0))])
        for lDescription in leafDescriptions:
            containerExtent.extend(lDescription.extent)

    if barrel and symmetrize:
        print(">> Symmetrizing the (barrel) container")
        zRange = containerExtent.range(acts.BinningValue.binZ)
        zMax = max(abs(zRange[0]), abs(zRange[1]))
        zExtent = acts.Extent([(acts.BinningValue.binZ, (-zMax, zMax))])
        containerExtent.extend(zExtent)

    print(">> Container has dimension",
          "r-range:", containerExtent.range(acts.BinningValue.binR),
          "z-range:", containerExtent.range(acts.BinningValue.binZ))

    zRangeContainer = containerExtent.range(acts.BinningValue.binZ)
    rRangeContainer = containerExtent.range(acts.BinningValue.binR)

    # Glue range of the container
    adapRange = rRangeContainer if barrel else zRangeContainer
    stayRange = zRangeContainer if barrel else rRangeContainer
    adaptBinningValue = acts.BinningValue.binR if barrel else acts.BinningValue.binZ
    stayBinningValue = acts.BinningValue.binZ if barrel else acts.BinningValue.binR

    # Complete the leafs for the barrel
    lastMax = adapRange[0]
    for ild, lDescription in enumerate(leafDescriptions):
        rangeLeaf = lDescription.extent.range(acts.BinningValue.binR) if barrel else lDescription.extent.range(
            acts.BinningValue.binZ)
        # Warning
        if rangeLeaf[0] < adapRange[0] or rangeLeaf[1] > adapRange[1]:
            print(">> Leaf:", lDescription.name,
                  "range:", rangeLeaf,
                  "outside container range:", adapRange)
            raise Exception("Leaf outside container")
        elif rangeLeaf[0] > lastMax:

            print(">> Container: introduce gap volume before leaf", lDescription.name,
                  "because leaf value", rangeLeaf[0], "is greater than last value", lastMax)
            gapExtent = acts.Extent([(stayBinningValue, stayRange),
                                     (adaptBinningValue, (lastMax, rangeLeaf[0]))])
            gapValues, gapTransform = extent_to_cylinder_parameters(gapExtent)
            print(">> Gap volume bound values =", gapValues)
            gapNode = acts.BlueprintNode.createLeafNode(
                lDescription.name + "_pre_gap", gapTransform, acts.VolumeBoundsType.Cylinder, gapValues, None, gapExtent)
            gapNode.geoIdGenerator = geoIdGenerator
            leafNodes.append(gapNode)

        # Get the surfaces
        surfacesHolder = acts.LayerStructureBuilder.SurfacesHolder(
            lDescription.surfaces)
        layerStructureConfig = acts.LayerStructureBuilder.Config()
        layerStructureConfig.binnings = lDescription.binning
        layerStructureConfig.surfacesProvider = surfacesHolder
        layerStructureConfig.auxiliary = lDescription.name+"_layer_structure"
        layerStructure = acts.LayerStructureBuilder(
            layerStructureConfig, lDescription.name, acts.logging.INFO)

        leafExtent = acts.Extent([(stayBinningValue, stayRange),
                                 (adaptBinningValue, rangeLeaf)])

        leafValues, leafTransform = extent_to_cylinder_parameters(
            leafExtent)
        print(">> Leaf volume bound values =", leafValues)

        leafNode = acts.BlueprintNode.createLeafNode(
            lDescription.name, leafTransform, acts.VolumeBoundsType.Cylinder, leafValues, layerStructure, leafExtent)
        leafNode.geoIdGenerator = geoIdGenerator
        leafNodes.append(leafNode)
        lastMax = rangeLeaf[1]

    return leafNodes, containerExtent


def build_cms_from_json(geoContext: acts.GeometryContext, logLevel : acts.logging.Level) -> acts.Detector:

    # detector in z
    detector_z = 300
    detectorExtent = acts.Extent([(acts.BinningValue.binZ, (100000, -100000)),
                                  (acts.BinningValue.binR, (100000, 0))])

    # Some options
    envelope_r = 2
    envelope_z = 2

    # Read the surfaces from the json file
    sOptions = acts.examples.SurfaceJsonOptions()
    sOptions.inputFile = "CMS_sensitive_mod.json"
    sOptions.jsonEntryPath = ["surfaces"]
    surfaces = acts.examples.readSurfaceVectorFromJson(sOptions)

    rzKdtBinning = [acts.BinningValue.binZ, acts.BinningValue.binR]

    rzKdtSurfaces = acts.KdtSurfacesDim2Bin100(
        geoContext, surfaces, rzKdtBinning)

    viewConfig = acts.ViewConfig()

    # Beam pipe section
    beamPipeRmax = 25
    beamPipeZmax = 600
    beamPipeExtent = acts.Extent([(acts.BinningValue.binZ, (-beamPipeZmax, beamPipeZmax)),
                                  (acts.BinningValue.binR, (0, beamPipeRmax))])
    beamPipeValues, beamPipeTransform = extent_to_cylinder_parameters(
        beamPipeExtent)
    beamPipeNode = acts.BlueprintNode.createLeafNode(
        "BeamPipe", beamPipeTransform, acts.VolumeBoundsType.Cylinder, beamPipeValues, None, beamPipeExtent)
    detectorExtent.extend(beamPipeExtent)

    # Pixel section:
    pixZmax = 600
    pixBarrelEcSplit = 290
    pixRmin = beamPipeRmax
    pixRmax = 220
    pixExtent = acts.Extent([(acts.BinningValue.binZ, (-pixZmax, pixZmax)),
                               (acts.BinningValue.binR, (pixRmin, pixRmax))])

    # Negative endcap
    pixNegEcVolumeId = 11
    pixNegEcIdGeneratorConfig = acts.GeometryIdGenerator.Config()
    pixNegEcIdGeneratorConfig.containerMode = True
    pixNegEcIdGeneratorConfig.containerId = pixNegEcVolumeId
    pixNegEcIdGeneratorConfig.overrideExistingIds = True
    pixNegEcIdGeneratorConfig.passiveAsSensitive = True

    pixNegEcIdGenerator = acts.GeometryIdGenerator(
        pixNegEcIdGeneratorConfig, "PixelNegEcIdGenerator", logLevel)

    pixEndcapRmin = pixRmin
    pixEndcapRmax = pixRmax


    pixNegEcExtent = acts.Extent([(acts.BinningValue.binZ, (-pixZmax, -pixBarrelEcSplit)),
                                    (acts.BinningValue.binR, (pixEndcapRmin, pixEndcapRmax))])

    pixNegEcLeafDescriptions = [
        leafDescription("PixelNegD2", acts.RangeXDDim2(
            (-550, -450), (0, 200)), 2, 36, None, None, None),
        leafDescription("PixelNegD1", acts.RangeXDDim2(
            (-450, -350), (0, 200)), 2, 36, None, None, None),
        leafDescription("PixelNegD0", acts.RangeXDDim2(
            (-350, -290), (0, 200)), 2, 36, None, None, None),
    ]

    # Complete them
    pixNegEcLeafDescriptions = complete_leaf_description(
        geoContext, pixNegEcLeafDescriptions, rzKdtSurfaces, envelope_r, envelope_z, False)

    # Create the nodes
    pixNegEcLeafs, pixNegEcExtent = create_leaf_nodes(
        geoContext, pixNegEcLeafDescriptions, None, pixNegEcExtent, False, False)
    pixNegEcVals, pixNegEcTransform = extent_to_cylinder_parameters(
        pixNegEcExtent)

    pixNegEcNode = acts.BlueprintNode.createBranchNode(
        "PixelNegativeEndcap",
        pixNegEcTransform,
        acts.VolumeBoundsType.Cylinder,
        pixNegEcVals,
        [acts.BinningValue.binZ],
        pixNegEcLeafs,
        pixNegEcExtent)
    pixNegEcNode.geoIdGenerator = pixNegEcIdGenerator

    # Barrel
    pixBarrelVolumeId = 10
    pixBarrelIdGeneratorConfig = acts.GeometryIdGenerator.Config()
    pixBarrelIdGeneratorConfig.containerMode = True
    pixBarrelIdGeneratorConfig.containerId = pixBarrelVolumeId
    pixBarrelIdGeneratorConfig.overrideExistingIds = True
    pixBarrelIdGeneratorConfig.passiveAsSensitive = True

    pixBarrelIdGenerator = acts.GeometryIdGenerator(
        pixBarrelIdGeneratorConfig, "PixelBarrelIdGenerator", logLevel)

    pixBarrelZmax = pixBarrelEcSplit
    pixBarrelRmin = 25
    pixBarrelRmax = pixRmax
    pixBarrelExtent = acts.Extent([(acts.BinningValue.binZ, (-pixBarrelZmax, pixBarrelZmax)),
                                     (acts.BinningValue.binR, (pixBarrelRmin, pixBarrelRmax))])
    pixBarrelLeafDescriptions = [
        leafDescription("PixelL0", acts.RangeXDDim2(
            (-290, 290), (15, 35)), 8, 12, None, None, None),
        leafDescription("PixelL1", acts.RangeXDDim2(
            (-290, 290), (40, 80)), 8, 32, None, None, None),
        leafDescription("PixelL2", acts.RangeXDDim2(
            (-290, 290), (85, 150)), 8, 48, None, None, None),
        leafDescription("PixelL3", acts.RangeXDDim2(
            (-290, 290), (150, 200)), 8, 64, None, None, None),
    ]

    # Complete them
    pixBarrelLeafDescriptions = complete_leaf_description(
        geoContext, pixBarrelLeafDescriptions, rzKdtSurfaces, envelope_r, envelope_z, True)

    # Create the nodes
    pixBarrelLeafs, pixBarrelExtent = create_leaf_nodes(
        geoContext, pixBarrelLeafDescriptions, None, pixBarrelExtent, True, True)
    pixBarrelVals, pixBarrelTransform = extent_to_cylinder_parameters(
        pixBarrelExtent)

    pixBarrelNode = acts.BlueprintNode.createBranchNode(
        "PixelBarrel",
        pixBarrelTransform,
        acts.VolumeBoundsType.Cylinder,
        pixBarrelVals,
        [acts.BinningValue.binR],
        pixBarrelLeafs,
        pixBarrelExtent)
    pixBarrelNode.geoIdGenerator = pixBarrelIdGenerator

    # Positive endcap
    pixPosEcVolumeId = 12
    pixPosEcIdGeneratorConfig = acts.GeometryIdGenerator.Config()
    pixPosEcIdGeneratorConfig.containerMode = True
    pixPosEcIdGeneratorConfig.containerId = pixPosEcVolumeId
    pixPosEcIdGeneratorConfig.overrideExistingIds = True
    pixPosEcIdGeneratorConfig.passiveAsSensitive = True

    pixPosEcIdGenerator = acts.GeometryIdGenerator(
        pixPosEcIdGeneratorConfig, "PixelNegEcIdGenerator", logLevel)

    pixEndcapRmin = pixRmin
    pixEndcapRmax = pixRmax

    pixPosEcExtent = acts.Extent([(acts.BinningValue.binZ, (pixBarrelEcSplit, pixZmax)),
                                    (acts.BinningValue.binR, (pixEndcapRmin, pixEndcapRmax))])

    pixPosEcLeafDescriptions = [
        leafDescription("PixelPosD0", acts.RangeXDDim2(
            (290, 350), (0, 200)), 2, 36, None, None, None),
        leafDescription("PixelPosD1", acts.RangeXDDim2(
            (350, 450), (0, 200)), 2, 36, None, None, None),
        leafDescription("PixelPosD2", acts.RangeXDDim2(
            (450, 550), (0, 200)), 2, 36, None, None, None),
    ]

    # Complete them
    pixPosEcLeafDescriptions = complete_leaf_description(
        geoContext, pixPosEcLeafDescriptions, rzKdtSurfaces, envelope_r, envelope_z, False)

    # Create the nodes
    pixPosEcLeafs, pixPosEcExtent = create_leaf_nodes(
        geoContext, pixPosEcLeafDescriptions, None, pixPosEcExtent, False, False)
    pixPosEcVals, pixPosEcTransform = extent_to_cylinder_parameters(
        pixPosEcExtent)

    pixPosEcNode = acts.BlueprintNode.createBranchNode(
        "PixelPositiveEndcap",
        pixPosEcTransform,
        acts.VolumeBoundsType.Cylinder,
        pixPosEcVals,
        [acts.BinningValue.binZ],
        pixPosEcLeafs,
        pixPosEcExtent)
    pixPosEcNode.geoIdGenerator = pixPosEcIdGenerator

    # Pixel
    pixVals, pixTransform = extent_to_cylinder_parameters(pixExtent)
    pixNode = acts.BlueprintNode.createBranchNode(
        "Pixel",
        pixTransform,
        acts.VolumeBoundsType.Cylinder,
        pixVals,
        [acts.BinningValue.binZ],
        [pixNegEcNode, pixBarrelNode, pixPosEcNode],
        pixExtent)

    # Print the detector node
    detectorExtent.extend(pixExtent)

    print(">> Detector extent",
          "r-range:", detectorExtent.range(acts.BinningValue.binR),
          "z-range:", detectorExtent.range(acts.BinningValue.binZ))

    # Build up
    detectorVals, detectorTransform = extent_to_cylinder_parameters(
        detectorExtent)
    detectorNode = acts.BlueprintNode.createBranchNode(
        "PixelBarrel",
        detectorTransform,
        acts.VolumeBoundsType.Cylinder,
        detectorVals,
        [acts.BinningValue.binR],
        [beamPipeNode, pixNode],
        detectorExtent)

    # Make the builder
    topBuilder = acts.CylindricalContainerBuilder.createFromBlueprint(
        detectorNode, acts.logging.INFO)

    # Build the detector
    geoIdGeneratorConfig = acts.GeometryIdGenerator.Config()
    geoIdGenerator = acts.GeometryIdGenerator(
        geoIdGeneratorConfig, "GeometryIdGenerator", acts.logging.INFO)

    detBuilderConfig = acts.DetectorBuilder.Config()
    detBuilderConfig.name = "CMS"
    detBuilderConfig.builder = topBuilder
    detBuilderConfig.geoIdGenerator = geoIdGenerator
    detBuilderConfig.auxiliary = "CMS from json"
    detBuilder = acts.DetectorBuilder(
        detBuilderConfig, "CMS builder", logLevel)

    return detBuilder.construct(geoContext)


if __name__ == "__main__":
    # Geometry context
    geoContext = acts.GeometryContext()
    detector = build_cms_from_json(geoContext, acts.logging.VERBOSE)

    # SVG style output
    surfaceStyle = acts.svg.Style()
    surfaceStyle.fillColor = [5, 150, 245]
    surfaceStyle.fillOpacity = 0.5

    surfaceOptions = acts.svg.SurfaceOptions()
    surfaceOptions.style = surfaceStyle

    viewRange = acts.Extent([])
    volumeOptions = acts.svg.DetectorVolumeOptions()
    volumeOptions.surfaceOptions = surfaceOptions

    # Transverse view
    xyRange = acts.Extent([[acts.BinningValue.binZ, [-50, 50]]])
    xyView = acts.svg.drawDetector(
        geoContext,
        detector,
        "cms",
        [[ivol, volumeOptions] for ivol in range(detector.numberVolumes())],
        [["xy", ["sensitives"], xyRange]],
    )
    xyFile = acts.svg.file()
    xyFile.addObjects(xyView)
    xyFile.write("cms_xy.svg")

    # Longitudinal view
    zrRange = acts.Extent([[acts.BinningValue.binPhi, [-0.1, 0.1]]])
    zrView = acts.svg.drawDetector(
        geoContext,
        detector,
        "odd",
        [[ivol, volumeOptions] for ivol in range(detector.numberVolumes())],
        [["zr", ["sensitives", "portals"], zrRange]],
    )
    zrFile = acts.svg.file()
    zrFile.addObjects(zrView)
    zrFile.write("cms_zr.svg")

