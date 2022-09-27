#!/usr/bin/env python3
import os
import acts
import acts.examples

def runTutorial(events=1, message="hello ACTS workshop"):

    # Sequencer
    s = acts.examples.Sequencer(
        events=events, numThreads=1, logLevel=acts.logging.INFO
    )
    
    # Add a single algorithm
    uaConfig = acts.examples.UserAlgorithm.Config(message = message)
    ua = acts.examples.UserAlgorithm(uaConfig, acts.logging.INFO)
    s.addAlgorithm(ua)
    
    return s


if "__main__" == __name__:

    runTutorial(3, "Hello ACTS workshop!").run()