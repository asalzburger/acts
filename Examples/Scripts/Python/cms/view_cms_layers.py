import acts
from acts import examples
import argparse

p = argparse.ArgumentParser()

p.add_argument(
    "-r", nargs=2, type=float, help="R range", default=(0.0, 1000.0)
)

p.add_argument(
    "-z", nargs=2, type=float, help="Z range", default=(0.0, 1000.0)
)

p.add_argument(
    "-o", "--output", default="CMS_layer", type=str, help="Output name"
)

args = p.parse_args()
# Draw context
geoContext = acts.GeometryContext()

# Read the surfaces from the json file
sOptions = acts.examples.SurfaceJsonOptions()
sOptions.inputFile = "CMS_sensitive_mod.json"
sOptions.jsonEntryPath = ["surfaces"]
surfaces = acts.examples.readSurfaceVectorFromJson(sOptions)

rzKdtBinning = [acts.BinningValue.binZ, acts.BinningValue.binR]

rzKdtSurfaces = acts.KdtSurfacesDim2Bin100(geoContext, surfaces, rzKdtBinning)

searchRange =  acts.RangeXDDim2(args.z, args.r)

searchSurfaces = rzKdtSurfaces.surfaces(searchRange)

viewConfig = acts.ViewConfig()
acts.examples.writeSurfacesObj(searchSurfaces, geoContext, viewConfig, args.output+".obj")
