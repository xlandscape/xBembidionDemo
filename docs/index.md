# Welcome to xBembidionDemo

xBembidionDemo is a spatially explicit, individual-based population model for the
ground beetle *Bembidion lampros* implemented as a component of the
[xLandscape](xLandscape/xLandscape-intro.md) modelling framework.

## Quick start

1. Copy `DLL\x64\Release\xBembidion.pyd` to `model\core\bin\python-3.9.7-amd64\DLLs\`.
2. Drag `template.xrun` onto `__start__.bat`.
3. Find results in `run\<SimID>\mcs\<mc_run_id>\store\arr.dat`.

For full installation and usage instructions, see the [README](https://github.com/xlandscape/xBembidionDemo/blob/main/README.md).

## Background

*Bembidion lampros* is one of the most common ground beetles in European
agroecosystems and a representative species for non-target arthropod (NTA) pesticide
risk assessment. The xBembidion model simulates its daily life cycle — development,
reproduction, movement, and mortality — driven by temperature and spatially explicit
landscape structure.

The biological model logic and parameters are derived from the
[ALMaSS Carabid B model](https://projects.au.dk/fileadmin/dmu.dk/en/animalsplants/almass/ALMaSS/Carabid_B/index.html)
(Topping 2009, Aarhus University).

## Model specification

See the **[Model Specification](model-specification.md)** for a full description of
the model in ODD format, including:

- State variables and scales
- Process overview and scheduling
- Biological parameters (development, reproduction, mortality, movement)
- Domain of applicability
- Known issues and constraints
- License and ALMaSS attribution analysis
