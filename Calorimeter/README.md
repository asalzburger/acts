# Calorimeter prototype

Optional calorimeter reconstruction for the `acts-nodd` development line, hosted
in ActsExamples. Detector descriptions and readout encodings belong to the
external [nODD repository](https://github.com/asalzburger/nodd). This package owns
reconstruction helpers, adapters, configuration and validation.

## Executable baseline

Enable `ACTS_BUILD_CALORIMETER=ON` in a normal ACTS build. The option defaults to
OFF and enables the existing basic Examples dependencies. It adds no new
third-party package, source download, detector requirement or Gaudi dependency.
The library targets are `Acts::Calorimeter` (framework-independent response) and
`Acts::ExamplesCalorimeter` (ActsExamples algorithms).
The `github-ci` preset enables this package and its CTest entries; the existing
coverage preset explicitly disables it to keep its Examples-free configuration.

```sh
cmake -S . -B build/calo -DACTS_BUILD_CALORIMETER=ON \
  -DACTS_BUILD_UNITTESTS=ON
cmake --build build/calo --target ActsExampleCalorimeter \
  ActsUnitTestCalorimeterResponse ActsUnitTestCalorimeterClustering
ctest --test-dir build/calo -R '^Calorimeter(Response|Clustering|Example)$' --output-on-failure
build/calo/bin/ActsExampleCalorimeter
```

Use the same compiler and dependency setup as the rest of ACTS. The example runs
three synthetic events through the real ActsExamples sequencer:

```text
SyntheticDeposits -> CalorimeterDigitizationAlgorithm -> CheckCalibratedHits
                  -> CalorimeterClusteringAlgorithm -> CheckClusters
```

Each event produces calibrated cells 42, 43 and 46 with energies 0.06, 0.04 and
0.06 GeV. The synthetic topology connects cells 42 and 43; cell 46 is isolated.
The two resulting clusters contain 0.10 and 0.06 GeV, conserving the accepted cell
energy. The executable checks both stages and returns failure on a mismatch.
It writes the framework's
timing CSV to the working directory. It neither loads nODD nor simulates showers;
its purpose is to establish a runnable reconstruction and event-store boundary.

## Data and response contract

All values use ACTS native units. Use `Acts::UnitLiterals`/`UnitConstants` when
converting values from another package, including time: do not assume numeric
nanoseconds already have the native ACTS time scale.

- `SimCalorimeterHit` contains a full 64-bit readout cell ID, the global cell
  centre, deposited energy and deposit time. It is a transient reconstruction
  input; it does not replace EDM4hep's persistent simulation data model.
- `CalorimeterHit` contains calibrated energy, an energy-weighted time and
  indices into the input deposit collection. Its position is a cell centre,
  never a shower-step position. Accepted deposits sharing an ID must use exactly
  the same centre, as obtained from one geometry lookup.
- `CalorimeterResponse` validates finite positions/times and finite nonnegative
  deposited energies. It accepts an inclusive deposit-time window, ignores
  zero-energy deposits, merges accepted deposits by cell, applies one positive
  global energy scale, then accepts positive cell energies at or above the
  calibrated threshold. The threshold is applied after merging. Output is
  sorted by cell ID; malformed data or numerical overflow causes failure.
- The time is weighted by accepted deposited energies; neither time of flight
  nor an event-time correction is applied. The scale is an explicit prototype
  parameter, not an established nODD calibration.
- Each event owns its output. The response has immutable configuration and no
  shared event state. Input indices remain meaningful only with the named input
  collection for that event; future podio adapters must create persistent
  relations explicitly.

## Clustering contract

`CalorimeterClusterer` implements a seeded connected-component baseline. Its
geometry-supplied `CellNeighbour` edges form an undirected graph copied and
normalized once at construction; duplicate/reversed edges are harmless, while
self-edges are rejected. Full readout IDs are preserved. Numeric proximity of IDs
does not imply adjacency. A cell missing from the graph is isolated; a geometry
cell without an accepted event hit cannot bridge components.

The algorithm keeps positive cells at or above the calibrated neighbour-energy
threshold and retains each connected component containing a cell at or above the
seed-energy threshold. The seed threshold must be at least the neighbour
threshold, and both must be finite and nonnegative. A component with multiple
seeds produces one cluster; a component whose total energy exceeds the seed
threshold but has no qualifying individual cell is dropped. Zero-energy and
below-threshold cells do not join or connect clusters.

Each retained cell appears exactly once. Cluster energy is the sum of calibrated
cell energies, with no second response or calibration. Position and time are
calibrated-energy-weighted cell means. The highest-energy cell identifies the
seed, with smaller IDs breaking ties. Clusters are ordered by their smallest
constituent ID; constituent indices and accumulation order follow cell-ID order.
These conventions make results reproducible under hit and edge permutations.

`CalorimeterCluster::hitIndices` refers to the input calibrated-hit collection
for that event. Following a hit's `sourceIndices` reaches simulated deposits.
The ActsExamples adapter writes a new cluster collection and preserves its
input. The native helper has no shared event state. Duplicate input cell IDs,
negative/nonfinite energies, nonfinite positions/times and output overflow are
errors. Links remain transient until the persistent IO milestone.

This baseline does not split nearby showers within one connected component,
perform noise-significance clustering, infer geometry, correct time of flight,
or define jet four-momenta. Layer/readout boundaries must be respected by the
geometry that supplies the edges. The synthetic graph is not a nODD geometry
model. Noise, sampling fluctuations, detector-specific response, saturation,
detailed electronics and shower-energy corrections are later milestones. No jet
or particle-flow performance is established by this synthetic example.

## Package layout

```text
Calorimeter/
  include/ActsCalorimeter/   # Framework-independent data and reconstruction
  src/Digitization/         # Deterministic response baseline
  src/Clustering/           # Seeded connected-component baseline
  Examples/                 # ActsExamples algorithms and runnable example
Tests/UnitTests/Calorimeter/ # Response and framework integration checks
```

Add geometry adapters, persistent IO, tracking and Pandora subdirectories
when the corresponding implementation lands. Keep dependencies optional at
their point of use; do not introduce empty backends or automatic downloads.

## Reviewable PR sequence

Each stage gets a dedicated branch pushed to `asalzburger/acts`. Stack dependent
PRs explicitly and keep validation evidence in their descriptions. PR 1 starts
from `codex/nodd-tracker-gen3`, whose existing PR provides the nODD integration.

| PR | Scope | Acceptance criterion | External decision |
| --- | --- | --- | --- |
| 1 (merged: #4) | Optional package, data contract, deterministic response, ActsExamples synthetic run | Enabled/disabled build checks, response tests and executable pass | No new packages |
| 2a | Calorimeter cluster baseline | Seed/neighbour boundaries, energy accounting, provenance, input-order invariance and sequencer checks | No new packages |
| 2b | Reconstructed calorimeter jets | Geometry/mass convention, FastJet constituent mapping and synthetic jet checks | Check with the user before introducing FastJet use; prefer the existing ACTS integration |
| 3 | EDM4hep/podio input, output and provenance | Real simulated-hit fixture round trip; preserve 64-bit IDs, units and relations | Confirm use of installed EDM4hep/podio before integration; no vendoring by default |
| 4 | nODD calorimeter geometry adapter and full-simulation input | Cells, centres, layers and neighbours agree with DD4hep; reproducible single-particle samples | Confirm geometry/input source and any needed external acquisition |
| 5 | ACTS track extrapolation to calorimeter entrance surfaces | Barrel/endcap states, covariance, failures and track-hit provenance validated | Reuse ACTS propagation; no Gaudi tracking wrapper |
| 6 | Direct PandoraSDK/LCContent ActsExamples adapter | Geometry/plugins/settings initialization, event conversion, PFO ownership and reliable reset; serial processing first | User approval required before fetching, linking or pinning Pandora dependencies |
| 7 | PFO jets, calibration and physics validation | Compare truth, calorimeter and PFO jets; single-particle response and pileup study with recorded inputs/configuration | Review any further response/clustering packages before adding them |

The first jet baseline can use synthetic calibrated cells; realistic performance
depends on the later geometry and simulation stages. Pandora's lepton-collider
content requires dedicated calibration and validation for proton collisions.

## Reuse policy

Use DD4hep/DDG4, EDM4hep/podio, FastJet and Pandora as library or IO interfaces
where appropriate. Key4hep calorimeter and Pandora packages are reference
implementations for adapters and response algorithms; their Gaudi algorithms
cannot be linked unchanged into this package. Any extracted implementation needs
a review of its license, attribution, framework coupling and detector assumptions.

The user requested a check-in before introducing externals. Researching an
interface or documenting a candidate does not authorize downloading, installing,
vendoring, pinning or linking it. Present the proposed package/version, purpose,
license and build impact for that decision. Existing ACTS dependencies used by
the basic Examples framework are sufficient for PR 1.
