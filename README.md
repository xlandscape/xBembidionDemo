# xBembidionDemo

xBembidionDemo is a spatially explicit, individual-based population model for the
ground beetle *Bembidion lampros*, implemented as a component of the
[xLandscape](https://github.com/xlandscape/LandscapeModel-Core) modelling framework.
The biological model is inspired by the ALMaSS Carabid B model (Topping 2009,
[documentation](https://projects.au.dk/fileadmin/dmu.dk/en/animalsplants/almass/ALMaSS/Carabid_B/index.html)).

---

## Table of Contents

1. [Requirements](#requirements)
2. [Installation](#installation)
3. [Running the model](#running-the-model)
4. [Configuration parameters](#configuration-parameters)
5. [Inputs and outputs](#inputs-and-outputs)
6. [Analyzing results](#analyzing-results)
7. [Model assumptions](#model-assumptions)
8. [Repository structure](#repository-structure)
9. [License and attribution](#license-and-attribution)

---

## Requirements

| Dependency | Version | Notes |
|---|---|---|
| Windows OS | 64-bit | The compiled DLL targets Windows x64 only. |
| Python | 3.9.7 (bundled) | Provided inside `model\core\bin\python-3.9.7-amd64\`. No separate installation required. |
| xBembidion DLL | — | Pre-built at `DLL\x64\Release\xBembidion.pyd`. **Must be copied** (see [Installation](#installation)). |
| GDAL | bundled | Used for landscape rasterization inside the Python component. |
| NumPy / Matplotlib | bundled | Used for raster handling and visualization. |

GDAL, NumPy, and Matplotlib are already available in the bundled Python environment provided by the xLandscape core. No `pip install` step is required for these packages.

---

## Installation

1. **Clone the repository** (including submodules):

   ```bat
   git clone --recurse-submodules https://github.com/xlandscape/xBembidionDemo.git
   ```

2. **Copy the DLL** into the bundled Python `DLLs` folder:

   ```bat
   copy DLL\x64\Release\xBembidion.pyd model\core\bin\python-3.9.7-amd64\DLLs\
   ```

   Without this step the simulation will fail with an `ImportError` on the `xBembidion` module.

3. **Verify the scenario** data is present in `scenario\NRW2\` (or your chosen scenario). Scenario folders must contain the landscape GeoPackage/shapefile and a `scenario.xproject` file.

---

## Running the model

**Drag `template.xrun` onto `__start__.bat`.**

This starts the xLandscape run-time and passes the parameter file to the model. A
command-prompt window will stay open and print progress messages. Simulation output is
written to `run\<SimID>\mcs\<mc_run_id>\store\arr.dat`.

> **Note — unique SimIDs required.**  
> Each run creates a folder named after `SimID`. If a folder with that name already
> exists, the run will fail. Either change `SimID` in `template.xrun` or delete the
> old results folder before re-running.

### What happens during a run

1. The xLandscape core reads `template.xrun` and resolves all variable substitutions.
2. The `LandscapeScenario` component loads field geometries and LULC codes from the scenario.
3. The `MarsWeather` component loads daily temperature from the scenario's weather CSV.
4. The `Bembidion` component:
   - Rasterizes the vector landscape to a 1 m² grid.
   - Constructs a synthetic pesticide exposure raster (top 20 % of the grid = lethal zone; placeholder implementation).
   - Initializes 100 adult beetles randomly placed in accessible habitat (LULC type 10).
   - Runs a daily time-step loop for 365 days. For years longer than 365 days the
     Julian day counter wraps back to day 1.
   - Writes the daily population size series to the xLandscape data store.

---

## Configuration parameters

Edit `template.xrun` to change these parameters:

| Parameter | Default | Description |
|---|---|---|
| `Project` | `scenario/NRW2` | Path to the landscape scenario folder. |
| `BembidionScenario` | `xBembidion-demo.xml` | Bembidion scenario XML inside `Bembidion\`. |
| `SimID` | `Bembidion-demo-scenario` | Unique identifier for the simulation run. |
| `NumberMC` | `1` | Number of Monte Carlo replicates. |
| `ParallelProcesses` | `1` | Replicates run in parallel (limited by CPU cores). |
| `SimulationStart` | `2000-01-01` | First simulated date. Must fall within the scenario's weather range. |
| `SimulationEnd` | `2001-12-31` | Last simulated date. |

---

## Inputs and outputs

### Inputs (wired in `model\variant\mc.xml`)

| Input | Source | Description |
|---|---|---|
| `Fields` | LandscapeScenario | Integer IDs for each spatial feature. |
| `FieldGeometries` | LandscapeScenario | WKB-encoded polygons of each feature. |
| `GeometryCrs` | LandscapeScenario | Coordinate reference system (Proj4/WKT). |
| `Extent` | LandscapeScenario | Bounding box in metres. |
| `LandUseLandCoverTypes` | LandscapeScenario | LULC code for each feature. |
| `AirTemperature` | MarsWeather | Daily average air temperature (°C) time series. |

### Outputs

| Output | Description |
|---|---|
| `NumberBembids` | Daily population size (number of individuals) across all life stages, stored as a list in the xLandscape data store (`arr.dat`). |

A matplotlib figure showing beetle movement traces overlaid on the LULC raster is
displayed (not saved) at the end of each Monte Carlo run.

---

## Analyzing results

Output is stored in HDF5 format at:

```
run\<SimID>\mcs\<mc_run_id>\store\arr.dat
```

### Viewing raw output

Use [HDFView](https://portal.hdfgroup.org/downloads/index.html) to browse the file.
Expand the `Bembidion` group to find `NumberBembids`.

### Reading with Python

```python
import h5py
import matplotlib.pyplot as plt

with h5py.File(r"run\Bembidion-demo-scenario\mcs\<mc_run_id>\store\arr.dat", "r") as f:
    counts = f["Bembidion/NumberBembids"][:]

plt.plot(counts)
plt.xlabel("Simulation day")
plt.ylabel("Population size")
plt.title("Bembidion population dynamics")
plt.show()
```

Install the required packages in your local Python environment before running analysis
scripts:

```bat
pip install h5py matplotlib numpy pandas
```

The `analysis\` folder contains example Jupyter notebooks designed for the
xCropProtection component output format. They can be adapted for xBembidion output
by pointing `arr.dat` at the Bembidion results path.

### Simulation log

Each run writes a plain-text log to `simulation_log.txt` in the working directory,
recording daily population sizes and mortality counts.

---

## Model assumptions

The following assumptions are built into the current implementation.
See [docs/model-specification.md](docs/model-specification.md) for a full model
description.

- **Species**: *Bembidion lampros* Herbst (Carabidae). Parameters are taken from the
  ALMaSS Carabid B model (Topping 2009) based on laboratory and field data from the
  literature.
- **Landscape**: a 2-D raster at 1 m² pixel resolution derived from vector input
  geometries. Features with LULC type `10` are treated as accessible habitat.
- **Temperature**: spatially uniform daily average temperature drives development,
  reproduction, and movement. Wind and precipitation are not used.
- **Population initialisation**: 100 adults placed randomly in accessible cells at
  the start of the simulation.
- **Pesticide exposure**: a static synthetic exposure raster is used (top 20 % of
  the landscape is a lethal zone). This is a placeholder for future integration with
  xCropProtection.
- **Time-step**: one day. Julian day wraps at 365 regardless of simulation length.
- **Spatial boundary**: beetles that would exit the landscape boundary are blocked by
  a random direction change (up to 10 retries).

---

## Repository structure

```
xBembidionDemo/
├── Bembidion/                  # Bembidion scenario XML and PPM calendar inputs
│   ├── xBembidion-demo.xml
│   └── PPMCalendars/
├── DLL/                        # C++ beetle model source and pre-built DLL
│   ├── xBembidion.h            # Constants, class declaration
│   ├── xBembidion.cpp          # Individual-based beetle logic + Boost.Python bindings
│   └── x64/Release/xBembidion.pyd   # Compiled Windows DLL (copy to Python DLLs/)
├── analysis/                   # Example Jupyter notebooks and requirements
├── docs/                       # MkDocs documentation sources
│   ├── model-specification.md  # Full model specification (ODD-style)
│   └── xLandscape/             # xLandscape framework documentation
├── model/
│   ├── core/                   # xLandscape core (submodule)
│   └── variant/
│       ├── Bembidion/
│       │   └── Bembidion.py    # Python xLandscape component
│       ├── experiment.xml      # Multi-MC experiment configuration
│       ├── mc.xml              # Single Monte Carlo run configuration
│       └── package.xsd         # Landscape package XML schema
├── scenario/
│   ├── NRW2/                   # North-Rhine Westphalia landscape scenario
│   └── NRW3/
├── __start__.bat               # Entry point – drag template.xrun onto this
├── template.xrun               # User-editable run parameters
├── LICENSE.txt                 # CC0 1.0 Universal
└── README.md                   # This file
```

---

## License and attribution

This software is released under the **CC0 1.0 Universal** public domain dedication.
See [LICENSE.txt](LICENSE.txt).

The biological model logic and parameters are derived from the **ALMaSS Carabid B**
model developed by Chris J. Topping, Department of Wildlife Ecology, National
Environmental Research Institute, Aarhus University:

> Topping, C.J. (2009). *ALMaSS Bembidion model documentation.*
> Available at: https://projects.au.dk/fileadmin/dmu.dk/en/animalsplants/almass/ALMaSS/Carabid_B/index.html

Parameter values and algorithmic logic for development, reproduction, mortality, and
movement are taken from this model and the corresponding primary literature cited
therein (principally Boye Jensen 1990 for thermal development thresholds).

**See [docs/model-specification.md](docs/model-specification.md) for the full license
and attribution discussion.**