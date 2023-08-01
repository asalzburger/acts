#!/usr/bin/env python3
import acts
import os

from acts.examples import (
    WhiteBoard,
    AlgorithmContext,
    CsvTrackingGeometryWriter,
    ObjTrackingGeometryWriter )

if "__main__" == __name__:

    trackingGeometry = acts.createSingleCylinderGeometry(34, 400, 100, 36)

    eventStore = WhiteBoard(name=f"EventStore#0", level=acts.logging.INFO)
    ialg = 0

    context = AlgorithmContext(0, 0, eventStore)
    
    writer = ObjTrackingGeometryWriter(
                level=acts.logging.VERBOSE, outputDir="."
            )
    writer.write(context, trackingGeometry)
