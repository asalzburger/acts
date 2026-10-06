# nODD examples

This directory contains ACTS adapters and scripts for the external
[nODD repository](https://github.com/asalzburger/nodd). Detector definitions,
materials, module placements and services remain owned by that repository.
The first entry point is its combined preliminary pixel barrel/endcap compact.

`DD4hepNoddDetector` loads the DD4hep geometry and inherits the DDG4 detector
construction used by Geant4. This first adapter deliberately does not build an
ACTS tracking geometry or apply an ODD material map. Material recording only
requires the full DD4hep/Geant4 model. The selected nODD pixel baseline remains
preliminary, with its engineering and coverage limitations intact.

## Build and configure

Build the nODD repository's DD4hep factory and maintained combined compact first,
using its documented compatible DD4hep/ROOT environment. The default inputs are:

- `<nodd>/build/dd4hep/detector/pixel-detector/pixel-detector.xml` and its XML includes.
- `<nodd>/build/dd4hep/detector/libnODDPixelBarrel.{so,dylib}`.
- `<nodd>/build/dd4hep/detector/libnODDPixelBarrel.components`.

Enable `ACTS_BUILD_EXAMPLES_DD4HEP` and `ACTS_BUILD_PYTHON_BINDINGS` when building
ACTS. `ACTS_BUILD_EXAMPLES_GEANT4` and ROOT examples are also required for the
material recorder. The library and Python bindings follow the existing DD4hep
build option; a separate nODD dependency is not downloaded or linked at build
time. All new detector sources, binding source, Python helpers and scripts live
here; the top-level Examples and Python/Examples CMake files register them.

After activating the built ACTS runtime:

```sh
export NODD_PATH=/path/to/nodd
python - <<'PY'
from acts.examples.nodd import getNoddDetector
detector = getNoddDetector()
print(detector.config.xmlFileNames)
PY
```

`getNoddDetector(nodd_dir=..., build_dir=..., compact_file=...)` accepts explicit
overrides. Relative build directories are relative to the nODD checkout;
explicit compact paths are relative to the current directory. `NODD_PATH` is
used only when `nodd_dir` is omitted. The helper checks the input files, loads
the factory by absolute path and adds its component directory to
`DD4HEP_LIBRARY_PATH`, preserving existing entries. Keep the build and compact
compatible; changing the XML path does not rebuild the factory.

## Record pixel material

```sh
python Examples/Nodd/Scripts/nodd_material_recording.py \
  --nodd-dir "$NODD_PATH" \
  -n 10 -t 100 --seed 228 \
  --eta-range -4 4 --phi-range 0 360 \
  -o material/nodd_pixel
```

The installed command is also `nodd_material_recording.py`. Use `--build-dir`
for a nondefault nODD build, or `--compact` for an explicitly selected compact.
The output is `material/nodd_pixel.root`, tree `material_tracks`; the output
directory is created automatically. `--material-track-collection` changes both
the event-store collection and ROOT tree name. The output argument is a stem,
without the `.root` extension.

The pipeline mirrors `Examples/Scripts/Python/material_recording.py`: seed228,
point origin, uniform eta/phi generation, 1–10 GeV neutral geantinos, HepMC3
conversion, single-thread Geant4 material recording and ROOT material-track
writing with pre/post-step information and recalculated totals. Defaults are
1000 events and100 tracks per event. These are straight material probes, not
charged-track propagation, hit simulation or the luminous-region acceptance
study. Recording runs through the current external pixel model, including its
passive supports and effective services. No new geometry values are selected.

## Tests

From the ACTS checkout root in the installed ACTS Python environment:

```sh
pytest Examples/Nodd/Tests/test_nodd.py -v
NODD_PATH=/path/to/nodd pytest Examples/Nodd/Tests/test_nodd.py -v
```

Path/error tests run with the DD4hep nODD bindings. The external-model tests are
skipped when `NODD_PATH` is absent; with it they load the actual factory and run
eight material probes in a subprocess, checking the ROOT tree and nonzero finite
material totals. Geant4 is required for that recording test. Each subprocess
has its own DD4hep/Geant4 state.
