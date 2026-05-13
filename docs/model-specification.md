# xBembidion Model Specification

This document describes the xBembidion model in the style of the ODD (Overview,
Design, Detail) protocol (Grimm et al. 2006, 2020). It covers model purpose, state
variables, processes, design concepts, known issues, and a license compliance
assessment.

---

## 1. Purpose

xBembidion simulates the population dynamics of *Bembidion lampros* Herbst
(Carabidae), a flightless spring-breeding ground beetle common in European agroecosystems.
The model is used to evaluate the effects of landscape structure, temperature-driven
phenology, pesticide exposure, and land management on beetle population size and spatial
distribution at the field and landscape scale.

The primary intended applications are:

- Pesticide risk assessment for non-target arthropods (NTA) at higher tiers.
- Evaluation of the impact of landscape structure and agricultural management on
  ground beetle population dynamics.
- Integration with pesticide exposure models (e.g., xCropProtection) within the
  xLandscape framework.

---

## 2. State variables and scales

### 2.1 Individual beetle (`TBembidion`)

Each beetle is an object with the following state variables:

| Variable | Type | Description |
|---|---|---|
| `X`, `Y` | `double` | Spatial position in landscape coordinates (metres). |
| `Direction` | `int` | Walking direction, 0–7 (0 = North, clockwise, 45° increments). |
| `LifeStage` | `int` | Current life stage: 0=egg, 1=larva I, 2=larva II, 3=larva III, 4=pupa, 5=adult. |
| `Age` | `int` | Total age in simulation days. |
| `DegreeDays` | `double` | Accumulated thermal units (°C·days) for the current pre-adult stage. |
| `FrostDegreeDays` | `double` | Accumulated frost degree days (negative values) during winter, used for winter mortality. |
| `CanReproduce` | `bool` | Whether the adult has completed at least one hibernation and may lay eggs. |
| `NumberEggsLeftInLife` | `int` | Lifetime egg quota remaining (initialised to 256). |
| `Hibernating` | `bool` | Whether the beetle is currently in hibernation. |
| `BodyBurden` | `vector<double>` | Accumulated pesticide concentration per active substance. |
| `BodyBurdenThreshold` | `vector<double>` | Lethal threshold per active substance (set externally). |
| `RandomWalkTarget` | `int` | Current movement target (forage or hibernation habitat). |
| `JulianDay` | `int` | Current Julian day (1–365, wrapping). |
| `Temperature` | `double` | Current daily average temperature (°C). |

### 2.2 Population

The beetle population is managed as a Python list (`self.Bembidions`) by the
`Bembidion` xLandscape component. All life stages are stored in the same list.

### 2.3 Landscape

The landscape is a 2-D raster at **1 m² resolution**, derived by rasterizing the
vector geometries from the xLandscape scenario. Each cell carries a LULC type code.
The current habitat interpretation is:

| LULC code | Habitat class | Interpretation |
|---|---|---|
| 10 | `HABITAT_ACCESSIBLE_AS_TARGET` | Open agricultural field; beetles can occupy and traverse. |
| other | `HABITAT_INACCESSIBLE` | Non-agricultural area; beetles cannot move there. |

> **Note:** The LULC look-up table that maps scenario codes to habitat classes is
> marked as TODO in the source (`Bembidion.py` line 376). Currently only LULC = 10
> is recognized as accessible habitat.

### 2.4 Environment

| Variable | Unit | Description |
|---|---|---|
| `AirTemperature` | °C | Daily average air temperature, provided as a 1-D time series by the `MarsWeather` component. Temperature is spatially uniform. |
| `PesticideAmountRaster` | dimensionless | Spatial pesticide exposure raster. Currently a placeholder: the top 20 % of the raster extent is assigned a value of 1 (lethal); the remainder is 0. |

### 2.5 Spatial and temporal scales

| Attribute | Value |
|---|---|
| Spatial resolution | 1 m² per pixel |
| Landscape extent | Variable (defined by scenario) |
| Time step | 1 day |
| Simulation period | User-defined via `SimulationStart`/`SimulationEnd` in `template.xrun` |
| Julian day wrapping | Day counter resets to 1 after day 365, regardless of actual calendar |

---

## 3. Process overview and scheduling

Each simulation day executes the following steps **for every individual** in the
population list (order is deterministic within a day; individuals are not shuffled):

1. **Update environment pointers** — habitat raster, density raster, pesticide raster,
   Julian day, temperature, and degree days are refreshed for the individual.
2. **Develop** — pre-adult individuals check whether the accumulated degree days
   exceed the threshold for their stage and advance to the next stage if so.
3. **Reproduce** — adult females (CanReproduce = true) lay eggs into the population
   list if temperature ≥ 3 °C and they are outside the hibernation period.
4. **Die** — mortality is evaluated in order: background, density-dependent,
   temperature-dependent (larvae), pesticide body burden, and cumulative winter
   mortality (evaluated once at end of hibernation, Julian day 90).
5. **Move** — living individuals move up to 14 m per day using a correlated random
   walk (see Section 5.3).

After processing all individuals, the daily population count is appended to the
output list.

---

## 4. Design concepts

### 4.1 Basic principles

xBembidion uses an **individual-based model (IBM)** at the resolution of individual
beetles. Emergent properties of interest are daily and annual population size, spatial
distribution of beetles, and mortality sources.

### 4.2 Emergence

Population dynamics emerge from individual-level birth (egg production), development
(thermal accumulation), death (multiple stressors), and movement processes interacting
with the spatially heterogeneous landscape and temperature forcing.

### 4.3 Adaptation

Individual beetles do not adapt. All decision rules are fixed (e.g., turning
probability, movement distance).

### 4.4 Sensing

- Adults sense local habitat suitability at their current and potential next position
  (1 m resolution).
- Individuals sense the pesticide amount at their current position.
- Adults sense local beetle density within a ±3 m neighbourhood to trigger
  density-dependent mortality.
- All individuals receive the current Julian day and air temperature from the
  population manager.

### 4.5 Stochasticity

Stochasticity is central to the model:

- **Initial placement**: beetles are placed at randomly chosen accessible cells.
- **Movement direction**: at each step there is a probability of turning ±45°
  (`movADULT_TURNING_PROBABILITY = 0.8`), and inaccessible cells force a random
  direction change.
- **Mortality**: background, density, temperature, and pesticide mortality are all
  evaluated against uniform random draws.
- **Reproduction**: the number of eggs produced per day is deterministic given
  temperature, but the initial beetle positions are stochastic.
- **Random number generation**: uses Mersenne Twister (`std::mt19937`) seeded by
  `std::random_device` at program start. The seed is **not configurable**, meaning
  replicate runs may differ.

### 4.6 Collectives

No collectives are used beyond the population list.

### 4.7 Observation

- The output `NumberBembids` records the daily total population size across all life
  stages.
- A matplotlib figure of beetle movement traces over the LULC raster is displayed
  (not saved by default) at the end of each run.
- A plain-text log (`simulation_log.txt`) records population counts and death counts
  per day.

---

## 5. Details

### 5.1 Development (thermal accumulation)

Development is driven by accumulated degree days above a base temperature:

```
ΔDD = max(0, T − T₀)
```

where `T` is the daily average temperature (°C) and `T₀ = 3.0 °C` is the development
threshold.

At temperatures above the inflection temperature (`T_inf = 12.0 °C`), the degree day
increment is multiplied by a life-stage-specific correction factor:

| Life stage | Threshold (°C·days) | Correction factor (>12 °C) |
|---|---|---|
| Egg | 178.58 | 178.58 / 124.9 ≈ 1.43 |
| Larva I | 101.4 | 101.4 / 87.5 ≈ 1.16 |
| Larva II | 107.4 | 107.4 / 95.2 ≈ 1.13 |
| Larva III | 190.4 | 190.4 / 189.3 ≈ 1.01 |
| Pupa | 147.7 | 147.7 / 132.3 ≈ 1.12 |
| Adult | — | — |

Parameters sourced from Jensen (1990) as cited in Topping (2009).

### 5.2 Reproduction

Reproduction occurs outside the hibernation window (Julian days 270–90):

- A female must have completed at least one hibernation (`CanReproduce = true`).
- Temperature must be ≥ `repMIN_EGG_LAYING_TEMPERATURE = 3.0 °C`.
- Eggs produced per day:

  ```
  eggs = floor(0.5 + (T − 3.0) × 0.6)
  ```

  Maximum 10 eggs per day; maximum 256 eggs per lifetime.

Eggs are placed at the female's current position and enter the simulation as new
individuals at life stage 0 (egg).

### 5.3 Movement

Movement is implemented as a **correlated random walk** executed by the C++ `Move()`
function:

1. Movement is suppressed during the hibernation period (Julian days 270–90) and when
   temperature < 1.0 °C.
2. With probability `movADULT_TURNING_PROBABILITY = 0.8` the beetle turns 45° left or
   right.
3. The beetle attempts to step up to `max_distance = 14 m` in the current direction.
4. If the target cell is inaccessible or low-quality, the direction changes randomly
   and up to 10 attempts are made.

> **Known issue**: the movement implementation calculates the new position as
> `X + max_distance × cos(angle)`, which applies the full 14 m distance in a single
> step rather than stepping 1 m at a time. This differs from the ALMaSS implementation
> and may cause beetles to skip over narrow habitat patches.

Forage (`Forage()`) and Aggregate (`Aggregate()`) behaviours are declared but not yet
implemented (empty stubs). The `HabitatSuitableForHibernation()` function is also
stubbed and always returns `false`, meaning hibernation aggregation movement never
completes successfully.

### 5.4 Mortality

Mortality is evaluated daily for every individual. Death from any single cause removes
the individual from the simulation. Causes are evaluated in order:

#### 5.4.1 Background mortality (all stages)

Daily probability of death:

| Stage | Probability |
|---|---|
| Egg | 0.007 |
| Larva I–III, Pupa, Adult | 0.001 |

#### 5.4.2 Density-dependent mortality (adults only)

Applied outside the hibernation period if the local adult density within ±3 m
exceeds 3 individuals. Daily death probability = 0.1.

#### 5.4.3 Temperature-dependent mortality (larvae only)

Mortality probability depends on temperature interval (5 °C bins, 0–25 °C):

| Temperature interval | Larva I | Larva II | Larva III |
|---|---|---|---|
| 0–5 °C | 0.0855 | 0.0626 | 0.0631 |
| 5–10 °C | 0.0855 | 0.0626 | 0.0631 |
| 10–15 °C | 0.1036 | 0.0940 | 0.0545 |
| 15–20 °C | 0.0563 | 0.0529 | 0.0268 |
| 20–25 °C | 0.0515 | 0.0492 | 0.0236 |
| >25 °C | 0.0657 | 0.0633 | 0.0295 |

#### 5.4.4 Pesticide mortality (all stages)

Pesticide body burden accumulates additively. Death occurs when body burden exceeds
the per-substance threshold. Thresholds are set externally by the Python component
(currently both set to 1.0, matching the binary exposure raster).

#### 5.4.5 Cumulative winter mortality (adults only)

Evaluated once per year at Julian day 90 (end of hibernation period) using
accumulated frost degree days (`FrostDegreeDays ≤ 0`):

| Frost degree days | Survival probability |
|---|---|
| > −40 | 0.94925 + FDD × 0.00426 |
| −40 to −80 | 1.16913 + FDD × 0.01149 |
| −80 to −107 | 0.6665 + FDD × 0.00528 |
| < −107 | 0.10 (90 % maximum mortality) |

### 5.5 Initialisation

```
InitialNumberOfIndividuals = 100   # hardcoded
SimulationDays = 365               # hardcoded (wrapping)
```

Adults are placed at randomly chosen cells by rejection sampling: a random position
is drawn and accepted if `0 ≤ x < width` and `0 ≤ y < height`. After the run begins,
individuals that are not in accessible habitat (LULC ≠ 10) are removed before the
first simulation day.

---

## 6. Domain of applicability

The model is calibrated for *Bembidion lampros* in northern temperate European
agroecosystems. The following conditions bound its domain:

- **Species**: *B. lampros* or ecologically similar flightless spring-breeding
  Carabidae with comparable life-history parameters.
- **Climate**: temperate northern Europe; the thermal development and winter mortality
  functions are calibrated for this climate zone.
- **Landscape**: agricultural landscapes where the relevant habitat is open arable
  fields (LULC type 10). Urban, forest, or semi-natural landscapes are not
  represented beyond being inaccessible barriers.
- **Spatial scale**: intended for field-to-landscape scales (1–100 km²). Very large
  landscapes may cause performance issues due to O(n·pixels) rasterization and O(n²)
  density map construction.
- **Time scale**: 1–5 year simulations are typical. Multi-decade runs may accumulate
  Julian-day drift because the 365-day wrap ignores leap years.

The model is **not validated** against empirical population data as part of this
implementation. Comparison with the original ALMaSS results is listed as a known
future requirement (see Section 7).

---

## 7. Known issues and constraints

The following limitations are documented in the source code or identified during code
review:

| # | Issue | Location | Severity |
|---|---|---|---|
| 1 | Population dynamics may be unrealistic; debugging not yet completed. | `Bembidion.py` class docstring | High |
| 2 | Field management events (ploughing, harvest) are not integrated; no field-event mortality is applied. | `Bembidion.py` class docstring | High |
| 3 | Hibernation aggregation and forage movement are empty stubs. Adults do not seek overwintering habitat or forage sites. | `xBembidion.cpp` `Forage()`, `Aggregate()`, `HabitatSuitableForHibernation()` | High |
| 4 | `HabitatIsSuitable()` always returns `true` — all habitats are treated as suitable regardless of LULC. | `xBembidion.cpp` line 501 | Medium |
| 5 | Movement applies the full 14 m displacement in one angular step instead of stepping 1 m at a time (mismatch with ALMaSS logic). | `xBembidion.cpp` `Move()` | Medium |
| 6 | Pesticide exposure is a hardcoded synthetic raster (top 20 % = lethal zone); real exposure data from xCropProtection is not yet wired. | `Bembidion.py` `run()` | High |
| 7 | LULC look-up table mapping scenario codes to habitat classes is not implemented; only LULC = 10 is accessible. | `Bembidion.py` line 376 (TODO comment) | Medium |
| 8 | Parameters are hardcoded (`InitialNumberOfIndividuals = 100`, `SimulationDays = 365`). No configuration file is used. | `Bembidion.py` `initialize()` | Low |
| 9 | Julian day wraps at 365 regardless of calendar year; leap years are not handled. Multi-year runs accumulate day-of-year drift. | `Bembidion.py` `simulate_day()` | Low |
| 10 | Random number generator seed is not user-controllable. Runs are not reproducible without fixing the seed in the C++ code. | `xBembidion.h` `GetRandomFloat()` | Low |
| 11 | Error handling in `Move()` silently resets the beetle to position (100, 100) on any exception. | `xBembidion.cpp` line 492 | Medium |
| 12 | `CanReproduce` for initialised adults is `true` from day 1 (allowing reproduction without hibernation in the first year), matching ALMaSS behaviour but this means first-year reproduction is not constrained by winter. | `xBembidion.cpp` constructor | Low |
| 13 | The model has not been validated against the original ALMaSS Bembidion results. | class docstring | High |
| 14 | Only Windows x64 is supported (DLL uses `windows.h`, `MessageBox`). | `xBembidion.h`, `xBembidion.cpp` | Medium |

---

## 8. References

- Grimm, V. et al. (2006). A standard protocol for describing individual-based and
  agent-based models. *Ecological Modelling*, 198(1–2), 115–126.
- Grimm, V. et al. (2020). The ODD protocol for describing agent-based and other
  simulation models: A second update to improve clarity, replication, and structural
  realism. *Journal of Artificial Societies and Social Simulation*, 23(2).
- Topping, C.J. (2009). *ALMaSS Bembidion model documentation.* National
  Environmental Research Institute, Aarhus University.
  https://projects.au.dk/fileadmin/dmu.dk/en/animalsplants/almass/ALMaSS/Carabid_B/index.html
- Jensen, L.B. (1990). Effect of temperature on the development of the immature stages
  of *Bembidion lampros* [Coleoptera: Carabidae]. *Entomophaga*, 35(2), 277–281.
  https://doi.org/10.1007/BF02374803
- Topping, C.J., Hansen, T.S., Jensen, T.S., Jepsen, J.U., Nikolajsen, F., Odderskær,
  P. (2003). ALMaSS, an agent-based model for animals in temperate European landscapes.
  *Ecological Modelling*, 167(1–2), 65–82.

---

## 9. License and ALMaSS attribution

### xBembidionDemo license

xBembidionDemo is released under the **CC0 1.0 Universal** public domain dedication
(see [`LICENSE.txt`](../LICENSE.txt)). To the extent possible under law, the authors
have waived all copyright and related rights in the software.

### ALMaSS-derived content

The biological model logic, parameter values, and algorithmic structure in
`DLL/xBembidion.cpp` and `DLL/xBembidion.h` are derived from the **ALMaSS Carabid B**
model. Cross-references to the original source are preserved in the C++ comments
(e.g., `Compare: File Bembidion_all.cpp - Line 363`).

**ALMaSS authorship:** Chris J. Topping, National Environmental Research Institute,
Aarhus University (2009).
