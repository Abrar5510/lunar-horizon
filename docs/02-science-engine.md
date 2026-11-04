# 02 — The Science Engine

> **TL;DR** — NASA/NAIF SPICE gives us the Sun and Earth direction from any lunar surface point to
> about 0.0001°. A horizon mask computed from LOLA terrain turns "direction" into "visible or not".
> From those two things we derive every number the app shows: illumination %, longest night,
> direct-to-Earth (DTE) %, DSN coverage, and solar power. The reference implementation is
> [`tools/lunar_sky.py`](../tools/lunar_sky.py). On 2026-10-20 it matched JPL Horizons to <0.0001°.
> The Unreal C++ module `LHScience` is a straight port of it.

This chapter is the **Validity** pillar of the judging rubric. Judges score whether a solution is
"grounded in scientific principles". This is where we prove it.

---

## 1. What the engine produces

| Output | Type (Unreal) | Meaning |
|---|---|---|
| Sun az/el | `FLHSkyState::SunAzDeg/SunElDeg` | Apparent direction of the Sun from the site (light-time + stellar aberration corrected) |
| Earth az/el | `FLHSkyState::EarthAzDeg/EarthElDeg` | Apparent direction of Earth's center |
| Sun visible fraction | `FLHSkyState::SunVisibleFraction` | 0..1 share of the solar disk above the local terrain horizon |
| Earth visible | `FLHSkyState::EarthVisible` | Earth center above terrain horizon |
| Earth phase | `FLHSkyState::EarthPhase` | Illuminated fraction of Earth seen from the Moon (drives Earth render + earthshine) |
| Distances | `SunDistanceKm`, `EarthDistanceKm` | For flux (1/r²) and Earth angular size |
| Illumination % | `FLHSiteMetrics::IlluminationPct` | Time-averaged visible solar fraction over the analysis window |
| Longest darkness | `LongestDarkHours` | Longest continuous run with < 1 % of the disk visible (battery sizing) |
| DTE % | `DTEPct` | Share of time Earth is above terrain **and** at least one DSN complex sees the Moon |
| DSN coverage % | `DSNCoveragePct` | Share of time at least one 70 m DSN antenna has the Moon ≥ 10° elevation |
| Average power | `AvgPowerW` | Mean output of a reference panel (default 1 m², 30 % efficient, vertical, Sun-tracking in azimuth) |
| Next sunrise/Earthrise | `NextSunriseUtc`, `NextEarthriseUtc` | Next transition from invisible to visible |

---

## 2. Why the south pole is strange (with our own verified numbers)

From `lunar_sky.py`, site 89.9°S 0°E, November 2026 (the hackathon month), checked against Horizons:

| Quantity | Value | Why |
|---|---|---|
| Sun elevation | **+1.44° → +1.55°** (Nov 10–17) | The Moon's spin axis is tilted only ~1.54° to the ecliptic normal, so at the pole the Sun circles the horizon. Over a year its elevation swings about ±1.5°. November 2026 is near the southern maximum. |
| Sun azimuth | Turns a full 360° in one synodic month (~29.5 d, ~12.2°/day) | The Sun circles the horizon once a lunar day |
| Earth elevation | **−6.7° → +6.4°** over one month | Optical libration in latitude, amplitude ≈ 6.68°, period ≈ 27.21 d (draconic month) |
| Earth azimuth | Stays within about ±6° of the 0° meridian direction | The sub-Earth point wanders near 0°E, 0°N (libration in longitude ≈ ±7.9°) |
| Earth angular diameter | 1.82°–2.01° (6,486–7,223 arcsec) | About 3.7× the Moon as seen from Earth |
| Sun angular diameter | ≈ 0.533° (radius 0.2665°) | So the Sun can be *partially* hidden by a ridge |

Two consequences drive the whole product:

1. **Horizontal solar panels are nearly useless.** At 1.5° elevation, cos(incidence) for a flat panel
   is sin(1.5°) ≈ 0.026. Polar landers use **vertical** arrays. A vertical panel that rotates to follow
   the Sun gets cos(1.5°) ≈ 0.9997 of full flux. The app must show this, because it is the first thing
   an educator will want to explain.
2. **During the hackathon weekend (Nov 14–15, 2026), Earth is 3.5°–4.5° *below* the horizon at the
   pole.** A lander there would be out of direct contact with Earth. This is a demo moment that writes
   itself.

---

## 3. SPICE kernel set

All files were downloaded and used on 2026-10-20 from
`https://naif.jpl.nasa.gov/pub/naif/generic_kernels/`.

| File | Path | Size | Purpose |
|---|---|---|---|
| `naif0012.tls` | `lsk/` | 5 KB | Leap seconds (UTC ↔ ET/TDB). Still current; no leap second since 2016. |
| `pck00011.tpc` | `pck/` | 131 KB | Body radii (Moon 1737.4 km, Earth ellipsoid) and orientation constants |
| `moon_pa_de440_200625.bpc` | `pck/` | 12.9 MB | High-accuracy lunar orientation (principal axes) from DE440 |
| `moon_de440_250416.tf` | `fk/satellites/` | 19 KB | Defines `MOON_PA` → `MOON_PA_DE440` and `MOON_ME` → `MOON_ME_DE440_ME421` |
| `de440s.bsp` | `spk/planets/` | 31 MB | Sun/Earth/Moon positions, 1849–2150 (short version of DE440) |
| `earthstns_itrf93_260814.bsp` | `spk/stations/` | 26 KB | DSN station positions (DSS-14, -43, -63 …) |
| `earth_topo_260814.tf` | `fk/stations/` | 68 KB | Station topocentric frames (`DSS-14_TOPO`, …) |
| `earth_latest_high_prec.bpc` | `pck/` | 5 MB | High-precision Earth orientation (`ITRF93`), needed for DSN elevations |

Total ≈ 50 MB. Ship it inside the packaged app under `Content/SPICE/` (see chapter 04).

Notes:
- **DE442** (`de442s.bsp`, posted 2025-02-06) exists. Its update targets the Uranus barycenter and it
  makes no lunar change that matters to us. JPL Horizons currently reports **DE441** in its headers.
  We use `de440s.bsp` because it pairs with the DE440 lunar orientation kernels. Our match against
  Horizons (§9) shows the choice is fine.
- **Do not** use `moon_080317.tf` + `moon_pa_de421_1900-2050.bpc` (the DE421 era) unless you are
  matching an older dataset on purpose.

### MOON_ME vs MOON_PA

| Frame | Definition | Who uses it |
|---|---|---|
| `MOON_ME` (mean Earth / polar axis) | +X points at the mean sub-Earth direction, +Z along the mean rotation axis | **LOLA DEMs, PGDA products, IAU/LRO cartography, Artemis site lists.** Our site coordinates. |
| `MOON_PA` (principal axes) | The Moon's inertia axes, which is what the ephemeris integrates | Internal to DE4xx |
| `IAU_MOON` | Low-accuracy analytic model | Avoid for sub-km work |

The kernel states the rotation between `MOON_ME` and `MOON_PA` is **≈ 0.02886°**. On the surface that is
0.02886° × π/180 × 1737.4 km ≈ **875 m**. Mixing the two frames silently moves your landing site
almost a kilometer. That is enough to put it on the wrong side of a ridge. Always pass `MOON_ME`.
JPL Horizons labels the same frame `MEAN_ME (high precision)`.

---

## 4. Conventions (shared contract)

- **Azimuth** is measured from local north, clockwise toward east, 0–360°. At a south-polar site,
  "north" means along the site's meridian toward the equator.
- **Elevation** is measured above the local tangent plane. SPICE's `ELLIPSOID` normal on the spherical
  Moon (R = 1737.4 km) is just the radial direction.
- **At exactly 90°S, north is undefined.** Store sites at their real longitude. For the pole itself use
  89.9°S, 0°E, which is what we validated against Horizons.
- **Time:** the UI shows UTC (ISO 8601). Internally we use SPICE ET (TDB seconds past J2000) via
  `str2et`. ET − UTC ≈ 69.18 s in 2026. If you ignore it, the Sun is off by ~0.009° and Earth by
  ~0.01°.
- **Apparent directions:** aberration correction `LT+S` (light time + stellar aberration). There is
  no refraction (no atmosphere).
- **Heights:** site height in meters above the 1737.4 km sphere. This is also the LOLA DEM datum
  (chapter 03).

---

## 5. Sun and Earth direction: `azlcpo`

CSPICE N0067 (Jan 2022, still the current toolkit) added `azlcpo_c`. It returns azimuth, elevation and
range of a target as seen from a surface point, with rates. One call replaces the old manual
"spkcpo + build ENU frame + recazl" recipe.

```c
void azlcpo_c ( ConstSpiceChar   * method,   // "ELLIPSOID"  (only supported value)
                ConstSpiceChar   * target,   // "SUN", "EARTH", "MOON"
                SpiceDouble        et,       // TDB seconds past J2000
                ConstSpiceChar   * abcorr,   // "LT+S"
                SpiceBoolean       azccw,    // SPICEFALSE → azimuth clockwise (navigation convention)
                SpiceBoolean       elplsz,   // SPICETRUE  → elevation positive toward +Z (up)
                ConstSpiceDouble   obspos[3],// observer, body-fixed, km
                ConstSpiceChar   * obsctr,   // "MOON"
                ConstSpiceChar   * obsref,   // "MOON_ME"
                SpiceDouble        azlsta[6],// range, az, el (rad), d/dt of each
                SpiceDouble      * lt );
```

Python (spiceypy 8.3.0, which wraps CSPICE N0067), from `tools/lunar_sky.py`:

```python
def site_vector_km(lat_deg, lon_deg, height_m=0.0):
    r = R_MOON_KM + height_m / 1000.0
    la, lo = math.radians(lat_deg), math.radians(lon_deg)
    return [r*math.cos(la)*math.cos(lo), r*math.cos(la)*math.sin(lo), r*math.sin(la)]

def az_el(sp, target, et, obspos, obsctr="MOON", obsref="MOON_ME"):
    azl, _lt = sp.azlcpo("ELLIPSOID", target, et, "LT+S", False, True, obspos, obsctr, obsref)
    return math.degrees(azl[1]), math.degrees(azl[2]), azl[0]
```

The same function gives **DSN elevation of the Moon**. Make the observer a station in `ITRF93` with
`obsctr="EARTH"`. Pass the station's ITRF93 position from `spkpos("DSS-14", et, "ITRF93", "NONE", "EARTH")`.
`ELLIPSOID` then uses Earth's WGS-84-like radii from `pck00011.tpc`, so the normal is the geodetic
vertical.

**Earth phase** (for rendering and earthshine) is the illuminated fraction
`k = (1 + cos(phase_angle)) / 2`. The phase angle is the Sun–Earth–Moon angle. Get it from
`phaseq_c(et, "EARTH", "SUN", "MOON", "LT+S")`, or from the dot product of the Earth→Sun and
Earth→Moon vectors.

---

## 6. Horizon masks: turning directions into visibility

A *horizon mask* is the terrain's elevation angle in every azimuth direction, seen from one point.
Once you have it, visibility at any time is one comparison: `body_el > mask(body_az)`. This is the
method behind the published LOLA illumination studies (Mazarico et al. 2011, *Icarus* 211,
1066–1081; Barker et al. 2021/2023). They compute horizon elevation along many azimuths from
polar DEMs, then derive illumination for any Sun geometry.

### 6.1 Algorithm

For each azimuth bin `a` (0.25° → 1,440 bins), march outward from the site and keep the maximum
elevation angle:

```
elev(d) = atan2( h(d) − h0 − d²/(2R),  d )        R = 1,737,400 m
mask[a] = max over d ∈ (0, dmax] of elev(d)
```

- `h0` = terrain height at the site **plus eye/panel height** (2 m for an astronaut eye, 1–3 m for a
  lander's arrays, 10 m for a mast; PGDA studies use 1 m and 5 m). Mast height matters a lot near the
  pole. Make it a UI slider: it shows planners why landers carry tall masts.
- `d²/(2R)` is the lunar curvature drop. It is 0.29 km at 10 km, 2.9 km at 100 km, and 6.5 km at
  150 km. Distant terrain only matters if it is very high (e.g. Malapert, Mons Mouton).
- `dmax` ≈ 150 km covers everything relevant at the pole. Use the 5 m/px site DEM for the first
  ~10 km and a coarser regional DEM (20–120 m/px) for the rest (chapter 03).
- **Step at half a pixel.** Our self-test caught this: at one-pixel steps the rounded sample positions
  skip thin ridges at some azimuths, and the mask gets holes. Use bilinear sampling or `0.5 × px` steps.

### 6.2 Direction in a polar stereographic grid (classic bug)

LOLA polar DEMs are south-polar stereographic. **Grid "up" (+y) is not local north.** At longitude λ:

```
north_grid = ( sin λ,  cos λ)      # moving north increases distance from the pole along the meridian
east_grid  = ( cos λ, −sin λ)
dir(az)    = cos(az)·north_grid + sin(az)·east_grid
```

Stereographic scale distortion at 85°S is k = 2/(1 + sin 85°) ≈ 1.002. That is negligible for
horizons inside the polar cap.

### 6.3 Reference implementation

`horizon_mask(dem, px_m, row0, col0, lon_deg, step_deg=0.25, max_km=150, eye_m=2.0)` in
`tools/lunar_sky.py` is vectorized NumPy. It runs one azimuth at a time, all distances at once.
1,440 azimuths × ~3,000 samples take about a second per site. `tools/make_horizon_masks.py`
(chapter 03) wraps it with GeoTIFF loading via rasterio and writes
`Content/Data/Horizons/<SiteId>.csv`:

```csv
azimuth_deg,elevation_deg
0.00,1.832
0.25,1.840
...
```

In Unreal, `ULHHorizonSubsystem` computes masks for **arbitrary clicked points** with the same loop:
a `ParallelFor` over azimuths against a downsampled float heightfield. A compute shader is a stretch
goal. Preset sites always use the precomputed CSV.

### 6.4 Partial solar disk

The Sun's radius is ~0.2665°, and grazing sunlight is the normal case at the pole. Treat the horizon
as locally flat across the disk. The visible fraction of a uniform disk whose center is `h` radii
above the horizon line is

```
h = (sun_el − mask(sun_az)) / sun_radius
f = 1                                   if h ≥ 1
f = 0                                   if h ≤ −1
f = 1 − [acos(h) − h·√(1−h²)] / π       otherwise      (circular-segment area)
```

`f` is `SunVisibleFraction`. It fades the directional light, so sunrise over a crater rim looks
gradual and physically right. It does not just switch on and off. The self-test checks
f(0) = 0.5 and f(0.5) ≈ 0.8045.

For **Earth** (~1.9° across) we report the center above or below the horizon. The same segment
formula gives a partial Earthrise for the visuals.

---

## 7. Metrics

All metrics come from one time sweep over the analysis window: default 30 days at 30-minute steps;
"mission planning" mode is 1 year at 10 minutes.

| Metric | Definition |
|---|---|
| `IlluminationPct` | mean(f) × 100 |
| `LongestDarkHours` | longest run with f < 0.01, × step |
| `DSNCoveragePct` | share of steps where max over {DSS-14 Goldstone, DSS-43 Canberra, DSS-63 Madrid} of Moon elevation ≥ 10° |
| `DTEPct` | share of steps where Earth is visible **and** DSN sees the Moon |
| `AvgPowerW` | mean of `P = S·(1 AU/r)²·A·η·f·cos(sun_el)` (vertical panel tracking in azimuth). S = 1361 W/m² (IAU 2015 B3 nominal), A = 1 m², η = 0.30 |

Panel models, selectable in the UI:

| Panel | Incidence factor | At sun_el = 1.5° |
|---|---|---|
| Vertical, azimuth-tracking (default) | cos(el) | 0.9997 |
| Vertical, fixed facing azimuth A | cos(el)·max(0, cos(sun_az − A)) | 0 to 1 over a lunar day |
| Horizontal (flat deck) | sin(el) | 0.026 |

**Battery sizing hint** (shown in the CompareCards tooltip):
`E_battery = P_survival × LongestDarkHours / depth_of_discharge`.

**Window matters.** Polar illumination depends on season (the Sun's elevation swings about ±1.5°
over ~346.6 days, the eclipse year) and on the 18.6-year nodal cycle. A site that looks 90 % lit in
November can be 60 % lit six months later. Always label metrics with their window. Offer
"this month", "mission window" and "full year".

**Eclipses.** When Earth passes in front of the Sun as seen from the Moon (a lunar eclipse seen from
Earth), the whole site goes dark for hours. Flag it with
`occult_c("SUN","ELLIPSOID","IAU_SUN","EARTH","ELLIPSOID","IAU_EARTH","LT+S","MOON",et,&code)` evaluated
at the Moon's center, which is accurate enough. The reference script omits this for brevity. Add it
before showing a full-year sweep.

Sample run (no terrain mask, 89.9°S 0°E, Nov 2026, 1,440 steps in 0.27 s):

```json
{ "IlluminationPct": 100.0, "LongestDarkHours": 0.0, "DTEPct": 47.9,
  "DSNCoveragePct": 98.1, "AvgPowerW": 417.3 }
```

Without terrain, the pole is "always lit". Real masks bring illumination down to the 60–90 % range
the literature reports for the best rims. That gap is the whole reason the app exists, so show it
with a "terrain on/off" toggle.

---

## 8. Time systems

| Layer | Representation |
|---|---|
| UI | UTC, ISO 8601 (`2026-11-14T18:00:00Z`), optional local time of the user |
| Unreal | `FDateTime` (UTC) in `ULHTimeSubsystem` |
| Science | ET (TDB seconds since J2000) from `str2et_c("2026-11-14T18:00:00")` |
| Fallback CSV | UTC column `utc` (same as `lunar_sky.py --csv`) |

Convert `FDateTime` → `"%Y-%m-%dT%H:%M:%S"` string → `str2et_c` once per frame. That is cheap.
Never accumulate ET from frame deltas, because that drifts. Always derive it from the absolute sim
time.

---

## 9. Validation (what we tell judges)

### 9.1 Ephemeris vs JPL Horizons, run on 2026-10-20

`python tools/lunar_sky.py --kernels <dir> --validate` queries the Horizons API for the Sun and Earth
from a topocentric site on the Moon (89.9°S, 0°E) and compares:

| UTC | Body | Ours az / el (°) | Horizons az / el (°) | Δ |
|---|---|---|---|---|
| 2026-11-10 00:00 | Sun | 175.4060 / 1.4446 | 175.4060 / 1.4446 | < 0.0001° |
| 2026-11-14 00:00 | Sun | 126.6196 / 1.4970 | 126.6196 / 1.4970 | < 0.0001° |
| 2026-11-17 00:00 | Sun | 90.0799 / 1.5532 | 90.0799 / 1.5532 | < 0.0001° |
| 2026-11-10 00:00 | Earth | 3.4436 / −6.7133 | 3.4436 / −6.7133 | < 0.0001° |
| 2026-11-14 00:00 | Earth | 358.7728 / −4.5483 | 358.7728 / −4.5483 | < 0.0001° |
| 2026-11-17 00:00 | Earth | 354.7850 / −0.8669 | 354.7850 / −0.8669 | < 0.0001° |

The script asserts worst-case error < 0.02°. Run it in CI and in the pitch video.

The Horizons request it uses:

```
https://ssd.jpl.nasa.gov/api/horizons.api?format=text&COMMAND='10'&EPHEM_TYPE=OBSERVER
  &CENTER='coord@301'&COORD_TYPE='GEODETIC'&SITE_COORD='0,-89.9,0'
  &START_TIME='2026-11-10'&STOP_TIME='2026-11-17'&STEP_SIZE='1 d'
  &QUANTITIES='4'&ANG_FORMAT=DEG&CSV_FORMAT=YES&EXTRA_PREC=YES
```

(`COMMAND='399'` for Earth. `SITE_COORD` is `E-lon, lat, alt_km`. Quantity 4 is airless apparent
az/el. Horizons uses the same north-clockwise azimuth.)

### 9.2 Horizon masks vs published illumination

- Compute `IlluminationPct` over the same window PGDA used (Jan 1 2024 – Jan 1 2026, 1 m and 5 m
  above the surface) for a PGDA site, e.g. Site01 "Connecting ridge" or Site04 "Shackleton rim".
  Compare with their published average illumination. Target within ±5 percentage points. Differences
  come from DEM resolution and dmax.
- Cross-check one sunrise time against a rendered Unreal frame. When the computed SunVisibleFraction
  passes 0.5, the rendered shadow terminator should cross the site. This doubles as a visual unit
  test.

### 9.3 Self-test without kernels

`python tools/lunar_sky.py --selftest` checks the disk-fraction formula and the horizon marcher on a
synthetic 1 km ring wall 10 km away. The expected horizon is atan((1000 − 28.8)/10000) = 5.547°.
It runs in a second.

---

## 10. The C++ port (`LHScience`)

The port is mechanical: same kernels, same calls, `double` throughout. Convert to `float` only at the
rendering boundary (light rotation).

```cpp
// LHSpice.h — the only file that includes SpiceUsr.h
#pragma once
#include "CoreMinimal.h"

struct FLHAzEl { double AzDeg = 0, ElDeg = 0, RangeKm = 0; };

class LHSCIENCE_API FLHSpice
{
public:
    static bool Init(const TArray<FString>& KernelPaths, FString& OutError);
    static double UtcToEt(const FDateTime& Utc);
    static bool AzEl(const char* Target, double Et, const double ObsPos[3],
                     const char* ObsCenter, const char* ObsFrame, FLHAzEl& Out);
    static bool StationPos(const char* Station, double Et, double OutPos[3]);   // ITRF93, km
private:
    static FCriticalSection Lock;   // CSPICE is NOT thread-safe: every call goes through this
    static bool CheckError(FString* OutError = nullptr);
};
```

```cpp
// LHSpice.cpp
#include "LHSpice.h"
THIRD_PARTY_INCLUDES_START
#include "SpiceUsr.h"
THIRD_PARTY_INCLUDES_END

FCriticalSection FLHSpice::Lock;

bool FLHSpice::Init(const TArray<FString>& KernelPaths, FString& OutError)
{
    FScopeLock Guard(&Lock);
    erract_c("SET", 0, (SpiceChar*)"RETURN");   // never abort() the game process
    errprt_c("SET", 0, (SpiceChar*)"NONE");     // we read messages ourselves
    for (const FString& P : KernelPaths)
    {
        furnsh_c(TCHAR_TO_UTF8(*P));
        if (CheckError(&OutError)) return false;
    }
    return true;
}

double FLHSpice::UtcToEt(const FDateTime& Utc)
{
    FScopeLock Guard(&Lock);
    SpiceDouble Et = 0;
    str2et_c(TCHAR_TO_UTF8(*Utc.ToString(TEXT("%Y-%m-%dT%H:%M:%S"))), &Et);
    CheckError();
    return Et;
}

bool FLHSpice::AzEl(const char* Target, double Et, const double ObsPos[3],
                    const char* ObsCenter, const char* ObsFrame, FLHAzEl& Out)
{
    FScopeLock Guard(&Lock);
    SpiceDouble Azl[6], Lt;
    azlcpo_c("ELLIPSOID", Target, Et, "LT+S", SPICEFALSE, SPICETRUE,
             ObsPos, ObsCenter, ObsFrame, Azl, &Lt);
    if (CheckError()) return false;
    Out.RangeKm = Azl[0];
    Out.AzDeg   = FMath::RadiansToDegrees(Azl[1]);
    Out.ElDeg   = FMath::RadiansToDegrees(Azl[2]);
    return true;
}

bool FLHSpice::StationPos(const char* Station, double Et, double OutPos[3])
{
    FScopeLock Guard(&Lock);
    SpiceDouble Lt;
    spkpos_c(Station, Et, "ITRF93", "NONE", "EARTH", OutPos, &Lt);
    return !CheckError();
}

bool FLHSpice::CheckError(FString* OutError)
{
    if (!failed_c()) return false;
    SpiceChar Msg[1841];
    getmsg_c("LONG", sizeof(Msg), Msg);
    UE_LOG(LogTemp, Error, TEXT("SPICE: %s"), UTF8_TO_TCHAR(Msg));
    if (OutError) *OutError = UTF8_TO_TCHAR(Msg);
    reset_c();
    return true;
}
```

The rest of `LHScience` is pure C++ math: the disk fraction, mask interpolation and metric sweep from
§6–7. Port it line for line and **unit-test it against `lunar_sky.py --csv` output** with an Unreal
Automation test (chapter 04 §15). The Python CSV is the oracle.

Performance: each sky state is ~5 SPICE calls (Sun, Earth, 3 DSN), about 0.2 ms in Python and less in
C++. A full year at 10-minute steps is 52,560 states, a few seconds on one worker thread. Run it as
an async task with progress reporting. Never run it on the game thread. The lock serializes SPICE, so
**more threads do not help**. If you need parallelism, precompute the CSV offline instead.

---

## 11. Gotchas checklist

- [ ] `MOON_ME`, not `MOON_PA` or `IAU_MOON` (875 m error).
- [ ] UTC → ET via `str2et`. Never `FDateTime` ticks plus a constant.
- [ ] `azccw = false`, `elplsz = true`, or your compass is mirrored.
- [ ] Polar stereographic grid direction uses the meridian formula in §6.2, not grid-up.
- [ ] Half-pixel steps or bilinear sampling in the horizon marcher.
- [ ] Eye/panel height added to h0.
- [ ] `erract_c("SET",0,"RETURN")`, or a bad kernel path kills the editor.
- [ ] All SPICE calls behind one lock.
- [ ] Label every metric with its time window.
- [ ] Kernels staged as non-UFS files in packaged builds (chapter 04 §5).

## Sources

- NAIF generic kernels index and the kernel comments quoted above: <https://naif.jpl.nasa.gov/pub/naif/generic_kernels/>
- `azlcpo_c` header: <https://naif.jpl.nasa.gov/pub/naif/toolkit_docs/C/cspice/azlcpo_c.html>
- Toolkit N0067: <https://naif.jpl.nasa.gov/naif/toolkit.html>
- spiceypy 8.3.0 (Oct 11, 2026): <https://pypi.org/project/spiceypy/>
- JPL Horizons API: <https://ssd-api.jpl.nasa.gov/doc/horizons.html>
- Mazarico et al. 2011, *Icarus* 211, 1066–1081; PGDA south-pole site products: <https://pgda.gsfc.nasa.gov/products/78>
- Libration amplitudes (6.68°, 7.9°): Gorkavyi et al., *Atmos. Meas. Tech.*; IAU 2015 Resolution B3 (1361 W/m²): <https://arxiv.org/abs/1605.09788>
- DSN Telecommunications Link Design Handbook 810-005, module 301 (coverage & geometry): <https://deepspace.jpl.nasa.gov/dsndocs/810-005/>
