#!/usr/bin/env python3
import os

import acts
import acts.examples

from acts.examples import GenericDetector, AlignedDetector

u = acts.UnitConstants


def runTutorial(trackingGeometry, field, outputDir, s=None, decorators=[]):
    s = s or acts.examples.Sequencer(events=100, numThreads=1)

    for d in decorators:
        s.addContextDecorator(d)

    rnd = acts.examples.RandomNumbers(seed=42)

    nav = acts.Navigator(trackingGeometry=trackingGeometry)

    stepper = acts.EigenStepper(field)

    print("We're running with:", type(stepper).__name__)
    prop = acts.examples.ConcretePropagator(acts.Propagator(stepper, nav))

    alg = acts.examples.PropagationAlgorithm(
        propagatorImpl=prop,
        level=acts.logging.INFO,
        randomNumberSvc=rnd,
        ntests=1000,
        sterileLogger=True,
        propagationStepCollection="propagation-steps",
    )

    s.addAlgorithm(alg)

    # Add a single algorithm
    uaConfig = acts.examples.UserAlgorithm.Config(message = 'User Algorithm embedded in Propagation example.')
    ua = acts.examples.UserAlgorithm(uaConfig, acts.logging.INFO)
    s.addAlgorithm(ua)
    
    # Output
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

    ## Generic detector: Default
    (
        detector,
        trackingGeometry,
        contextDecorators,
    ) = GenericDetector.create(mdecorator=matDeco)

    ## Magnetic field setup: Default: constant 2T longitudinal field
    field = acts.ConstantBField(acts.Vector3(0, 0, 2 * acts.UnitConstants.T))

    runTutorial(
        trackingGeometry, field, os.getcwd(), decorators=contextDecorators
    ).run()