#!/usr/bin/env python3
import os
import math

import acts
import acts.examples
from acts.examples import geant4

u = acts.UnitConstants

def runPropagation(trackingGeometry, field, outputDir, s=None, decorators=[], nTests = 10, phiRange = (-math.pi, math.pi), etaRange = (-2, 2),):
    s = s or acts.examples.Sequencer(events=100, numThreads=1)

    for d in decorators:
        s.addContextDecorator(d)

    rnd = acts.examples.RandomNumbers(seed=42)
    nav = acts.Navigator(trackingGeometry=trackingGeometry, level = acts.logging.INFO)

    stepper = acts.EigenStepper(field)
    # stepper = acts.AtlasStepper(field)
    # stepper = acts.StraightLineStepper()

    print("We're running with:", type(stepper).__name__)
    prop = acts.examples.ConcretePropagator(acts.Propagator(stepper=stepper, navigator=nav))

    alg = acts.examples.PropagationAlgorithm(
        propagatorImpl=prop,
        level=acts.logging.INFO,
        randomNumberSvc=rnd,
        ntests=nTests,
        sterileLogger=False,
        etaRange = etaRange,
        phiRange = phiRange,
        debugOutput = True,
        propagationStepCollection="propagation-steps",
    )

    s.addAlgorithm(alg)

    # Output
    s.addWriter(
        acts.examples.ObjPropagationStepsWriter(
            level=acts.logging.INFO,
            collection="propagation-steps",
            outputDir=outputDir + "/obj",
        )
    )

    s.addWriter(
        acts.examples.RootPropagationStepsWriter(
            level=acts.logging.INFO,
            collection="propagation-steps",
            filePath=outputDir + "/propagation_steps.root",
        )
    )

    return s


if "__main__" == __name__:
    matDeco = None

    nnbConfig = acts.examples.NNbarDetector.Config()
    nnbConfig.name = 'nnbar'
    nnbConfig.gdmlFile = 'nnbar_mod.gdml'
    nnbConfig.innerSensitiveMatches = [ 'ili' ]
    nnbConfig.tpcSensitiveMatches = [ 'TPC' ]

    nnb = acts.examples.NNbarDetector(nnbConfig)
    [ trackingGeometry, decorators, elements ] = nnb.trackingGeometry()
    
    ## Magnetic field setup: Default: constant 2T longitudinal field
    field = acts.ConstantBField(acts.Vector3(0, 0, 0 * acts.UnitConstants.T))

    seq = acts.examples.Sequencer(events=100, numThreads=1)

    runPropagation(
        trackingGeometry, 
        field, 
        os.getcwd(), 
        decorators=[], 
        nTests = 1000,
        phiRange = ( -math.pi, math.pi), 
        etaRange = ( -1.5, 1.5),
        s = seq
    ).run()
