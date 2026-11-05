# Martian Map

An interactive Mars globe in Unreal Engine 5.8, built on NASA MGS MOLA topography.

- **Globe**: MOLA 16 px/deg elevation displaces a 0.5° sphere (adjustable vertical exaggeration), with MOLA elevation colours or a natural rust look.
- **Places**: 10 landers/rovers, 13 major features and 5 water-ice sites. Click one to fly there.
- **Inspect any spot**: elevation, mean air pressure (and whether liquid water can exist there), local solar time, Sun elevation and sunlight.
- **Real sunlight and Mars time**: the Sun lights the globe from the true subsolar point. Mars Sol Date, Coordinated Mars Time and season (Ls) use the Mars24 algorithm, which agrees with JPL SPICE to 0.006° (`tools/mars_check/run.sh`).
- **Rover route planner**: set A and B to get a slope-limited A* route on the 4 px/deg grid (about 15 km cells), with distance, climb, steepest slope, drive time in sols and an elevation profile.

Controls: drag to pan, scroll to zoom, click to inspect.

## Setup (from the repo root)

```bash
.venv/bin/python tools/mars_prep.py
```

```bash
/Volumes/Transend/Apps/UE_5.8/Engine/Build/BatchFiles/Mac/Build.sh MarsMapEditor Mac Development -Project="$PWD/MarsMap/MarsMap.uproject" -WaitMutex
```

```bash
/Volumes/Transend/Apps/UE_5.8/Engine/Binaries/Mac/UnrealEditor-Cmd "$PWD/MarsMap/MarsMap.uproject" -run=pythonscript -script="$PWD/tools/mars_ue_setup.py" -unattended -nullrhi
```

Then open `MarsMap/MarsMap.uproject` and press Play. The game mode spawns everything, so there's no level to set up.

`mars_prep.py` downloads the MOLA grids (35 MB) and bakes the two textures. `mars_ue_setup.py` imports the textures and builds `M_Globe`.

Scripted views, for screenshots or a demo video, use console commands: `MarsGo <lat> <lonE> <altKm>`, `MarsRoute <latA> <lonA> <latB> <lonB>`, `MarsNatural 1` and `MarsShot <file.png>`.

## Known limits

- Route slopes are regional, measured between cells about 15 km apart. They say nothing about rocks or metre-scale slopes.
- Air pressure is an annual mean (610 Pa at datum, 11.1 km scale height). The real value swings about 25% with the seasons.
- Ice-site and feature coordinates are approximate centres.
