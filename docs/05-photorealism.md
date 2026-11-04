# 05 — Photorealism: Make It Look Like a Photo From the Pole

> **TL;DR** — Realism at the lunar south pole comes from physics, not detail. Use the true Sun
> (131,000 lux, 0.533° disk) at the real 0–1.5° elevation, a pitch-black sky, no haze, and shadows
> 150 km long. Add earthshine computed from Earth's phase (≈ 13 lux at full Earth), Lumen bounce
> light off sunlit crater walls, a regolith material with Lommel–Seeliger brightening and an
> opposition surge, a fixed Apollo-style exposure (EV100 ≈ 14.9 = f/11, 1/250 s, ISO 100), and stars
> that only appear when you switch to "dark-adapted eye". Get those right and LOLA terrain at
> 5 m/px looks like an LRO oblique. Get them wrong and no amount of 8K textures will save it.

---

## 1. What the real south pole looks like, and why

| Real phenomenon | Cause | How we reproduce it |
|---|---|---|
| Sun skims the horizon (0–1.5° elevation) | Lunar spin axis tilted only ~1.54° | SPICE-driven directional light (chapter 02) |
| Shadows tens to 150 km long; peaks lit, basins black | 4 km relief ÷ tan(1.5°) ≈ 153 km | VSM + far-field / ray-traced shadows (§4) |
| Razor-sharp but not infinitely sharp shadow edges | Sun is a 0.533° disk | `LightSourceAngle = 0.533` |
| Pitch-black sky in daylight; no stars in photos | No atmosphere; exposure set for sunlit ground | No SkyAtmosphere/fog; manual exposure |
| Distant mountains look as crisp as near ones | No aerial perspective | **No** height fog, no distance haze. Use scale cues instead (§7) |
| Shadowed crater floors aren't perfectly black | Sunlight bounced off sunlit walls (dominant, ~10× earthshine per ShadowCam) + earthshine | Lumen GI + earthshine light |
| Bright halo around your own shadow | Opposition surge (shadow hiding) | Phase-angle term in the regolith material (§5) |
| Surfaces look "flat", without limb darkening | Lommel–Seeliger scattering of regolith | View-dependent brightness term (§5) |
| Earth: blue-white disk ~1.9° across, near the horizon, bobbing ±6.7° over a month | Libration | `ALHCelestialRig` Earth mesh (chapter 04 §8) |

**Reference imagery** (keep a "real vs render" board and compare every day):
- LROC NAC oblique views of Shackleton and the connecting ridge (lroc.im-ldi.com).
- LROC NAC **low-Sun controlled mosaics** for each Artemis region (chapter 03). Render the same
  site at the same sub-solar longitude and compare side by side. This is also a killer slide.
- ShadowCam images of Shackleton's interior (2023 onward). This is what PSR floors look like under
  bounce light and earthshine.
- Chandrayaan-2 OHRC 0.25 m/px images of the Shackleton–de Gerlache ridge (boulders).
- Artemis II Earthset over the lunar limb, Apr 6 2026 (NASA image `art002e021278`, VERIFY the ID on
  images.nasa.gov). It is the cinematic reference for Earth near the horizon.
- Apollo surface photography (Lunar Surface Journal) for exposure and shadow character.

**Prior art to cite** (judges like "NASA does this too"):
- **DUST** (Digital Lunar Exploration Sites Unreal Simulation Tool), NASA JSC, MSC-27522-1. UE5
  south-pole visualization from LRO DEMs + SPICE ephemerides + synthetic textures + representative
  crater/rock distributions. That is essentially our recipe, but our tool is public and built for
  comparing sites.
- **NASA Langley "Lunar South Pole Lighting Testbed"** in UE5 for computer vision (ASCEND 2023,
  NTRS 20230013102).
- Hapke-BRDF regolith rendering for VR simulation (Chrono::Sensor; Colorado/NESS paper). It is the
  source of the photometric parameters in §5.

---

## 2. Scene setup: what to delete first

Delete or never add: **SkyAtmosphere**, **Exponential Height Fog**, **Volumetric Clouds**,
`ACesiumSunSky`, and the default SkyLight with a captured sky.

Add:
- `ALHCelestialRig` (Sun + earthshine lights, Earth mesh, star sphere), chapter 04 §8.
- One **Post Process Volume** (Infinite Extent / Unbound) with the exposure settings in §3.
- Optional **SkyLight** set to a *black* cubemap, intensity 0 (some features expect one). Physically
  there is no sky light.

---

## 3. Light and exposure (exact values)

### 3.1 Sun

| Property | Value | Source / reasoning |
|---|---|---|
| Intensity | **131,000 lux** × SunVisibleFraction × (1 AU / r)² | Extraterrestrial solar illuminance ≈ 128–134 klx |
| Source Angle | **0.533°** | 2·asin(695,700 km / 1 AU) = 0.5329° (UE's default 0.5357° is close) |
| Temperature | 5,778 K (or color ≈ 1.0, 0.98, 0.95) | No atmospheric reddening on the Moon. Sunlight stays white even at the horizon |
| Atmosphere Sun Light | **off** | No atmosphere |
| Cast Shadows | on; VSM; **Ray Traced Shadows** on in Cinematic preset | Long grazing shadows (§4) |
| Mobility | Movable | Driven every frame |

Note: **sunlight is not orange at sunrise on the Moon.** Teams always get this wrong. It is the
first "this looks fake" tell.

### 3.2 Earthshine (second directional light)

Earth at full phase reflects geometric albedo p ≈ 0.367 of sunlight from a disk of radius R⊕ at
distance d:

```
E_full = p · E_sun · (R⊕ / d)²  = 0.367 × 131,000 × (6,371 / 384,400)²  ≈ 13.2 lux   (≈ 1e-4 of sunlight)
E(α)   = E_full · Φ(α),   Φ(α) = [sin α + (π − α) cos α] / π       (Lambert-sphere phase law)
α      = Sun–Earth–Moon phase angle   (α = 0 ⇒ full Earth)
```

| Earth phase angle | 0° (full) | 45° | 90° (half) | 135° |
|---|---|---|---|---|
| Earthshine (lux) | 13.2 | 10.0 | 4.2 | 0.6 |

`EarthshineLux(phase)` in `ALHCelestialRig` uses exactly this. Color: slightly blue (≈ 6,500–8,000 K).
Source angle 1.9° (Earth's disk). Cast shadows **on**. Earthshine shadows inside PSR-adjacent
terrain are subtle and gorgeous. Turn it off when Earth is below the horizon (`EarthVisible`).

### 3.3 Exposure ("photographic" mode, the default)

Apollo crews shot sunlit surfaces at **f/11, 1/250 s**. With a digital ISO 100 sensor that is
EV100 = log2(11² × 250) = **14.9**. In the PPV:

- Exposure → Metering Mode **Manual**, **Apply Physical Camera Exposure** on.
- Camera: Shutter Speed 250 (1/250 s), Aperture f/11, ISO 100. Or set Exposure Compensation directly
  so the effective EV100 ≈ 14.5–15.
- Local Exposure: **off** in photographic mode (honest Apollo contrast).
- Bloom: Convolution or Standard at low intensity (0.3–0.5). The real Sun at 131 klx should flare a
  little when it peeks over a ridge, and a sunrise "star" over a crater rim is a hero shot.
- Lens flares: minimal. Vignette: 0.2. Film grain: off or very low.

### 3.4 "Dark-adapted eye" mode (toggle in the UI)

Human eyes in a suit visor, standing in shadow, see the earthshine-lit terrain and the stars that
cameras miss. Mode switch:
- EV100 → **6–8**. Sunlit areas blow out to white, so add **Local Exposure** (Highlight Contrast
  Scale ≈ 0.6, Shadow Contrast Scale ≈ 0.8) to keep them readable.
- Star sphere emissive calibrated so stars appear at EV100 ≲ 7 and vanish at ≥ 12.
- Label it clearly: "Simulated human vision. Cameras would not see this." That honesty earns
  Validity points.

### 3.5 "Visibility boost" (accessibility, clearly labeled)

For projectors and classrooms: multiply earthshine by 20–50× and add a tiny skylight. Always show
a badge "Enhanced visibility, not physical". Data-viz honesty again.

---

## 4. Shadows at grazing incidence

With the Sun at 0.5–1.5°, a 1 km ridge shades 40–115 km of terrain. Shadow maps hate this:
grazing angles, enormous extents, and acne on gentle slopes.

| Range from camera | Technique |
|---|---|
| 0–5 km | **Virtual Shadow Maps**. Keep `r.Shadow.Virtual.SMRT.RayCountDirectional` at 8 (Epic default) for clean penumbra. Lowering source angle is **wrong** here: we want the true 0.533° |
| 5–40 km | VSM clipmaps: the default `r.Shadow.Virtual.Clipmap.LastLevel` covers roughly 2^22 cm ≈ 42 km (VERIFY default in 5.8) |
| 40–150 km (regional Cesium terrain) | **Hardware ray-traced shadows** on the Sun (`Cast Ray Traced Shadows`), or distance-field shadows for non-Nanite meshes. Path tracer for stills |
| Hero stills / trailer | **Path Tracer** (Movie Render Graph): exact soft shadows, bounce and earthshine |

Shadow acne on the near-flat, near-grazing plains: raise the light's *Shadow Bias* a little
(0.6–1.0) and *Shadow Slope Bias*, check `r.Shadow.Virtual.NormalBias`, and compare against the
path tracer.

**Validation hook:** the analysis says `SunVisibleFraction` crosses 0.5 at the site at time T.
Render the site at T. The shadow terminator should be on the site. Show this in the demo, because
it proves the rendering and the math agree.

---

## 5. Regolith material: `M_Regolith` + `MF_RegolithPhotometry`

Lunar soil is not Lambertian. Two effects matter visually:

1. **Lommel–Seeliger (LS) brightening at grazing view.** Brightness ∝ μ₀/(μ₀ + μ)
   (μ₀ = cos incidence, μ = cos emission) instead of Lambert's μ₀. Relative to Lambert, the
   surface gets brighter by ∝ 1/(μ₀ + μ). Distant slopes viewed edge-on look flatter and brighter,
   and the full Moon has no limb darkening.
2. **Opposition surge**: a sharp brightening when the view direction approaches the light direction
   (phase angle g → 0). It is the halo around your own shadow.

UE's legacy Default Lit model is Lambert + GGX. We approximate Hapke without a custom shading model
by modulating **Base Color** with a view- and Sun-dependent factor. The Sun is the only significant
light, so we pass its direction through `MPC_Lunar.SunDirWS` (set by `ALHCelestialRig`):

```
μ0   = saturate(dot(N, SunDirWS))
μ    = saturate(dot(N, CameraVectorWS))
g    = acos(dot(SunDirWS, CameraVectorWS))                     // phase angle
LS   = clamp(1 / (μ0 + μ + 0.05), 0.5, 3.0) * 0.6              // normalized so LS≈1 at μ0=μ≈0.8
Bsh  = 1 + B0 / (1 + tan(g/2) / h)       B0 = 1.8, h = 0.071   // shadow-hiding opposition surge
BaseColor = Albedo * LS * lerp(1, Bsh, OppositionStrength)    // OppositionStrength ≈ 0.35, tune
```

B0 = 1.80 and h = 0.071 come from the Hapke fit used in Chrono::Sensor (w = 0.0326, b = 0.240,
c = 0.305, B_s0 = 1.80, h_s = 0.0715, θ̄ = 23.4°, valid 30°S–30°N). Treat them as a starting point:
opposition strength is tuned by eye against Apollo/LRO images.

Other inputs:

| Input | Value |
|---|---|
| Albedo | Start at linear **0.12** (the Moon's visual geometric albedo) and modulate ±30 % with the LOLA 1064 nm normal-albedo map (shadow-free, chapter 03). Calibrate by rendering a sunlit slope and comparing its brightness with an LROC NAC image at a similar Sun angle |
| Saturation | ~5 % warm-gray. Lunar highlands are almost neutral. Never brown |
| Roughness | 0.95–1.0 |
| Specular | 0.2–0.3 (kills the GGX sheen that looks "wet") |
| Normal | Macro normal from the DEM (5 m) + tiled micro-normal (regolith clods, footprints) blended by distance |
| Displacement | Nanite Tessellation for close-ups if it is stable in 5.8 (VERIFY status); otherwise parallax-occlusion on the detail layer only |

Limitations to state: the factor is approximate. Lumen's surface cache sees the view-dependent term
from its own capture angle, which is acceptable at this level. A proper Hapke shading model needs
Substrate or engine changes, so leave that for after the hackathon.

---

## 6. Surface detail: craters and boulders at human scale

A 5 m DEM has no rocks and no craters smaller than about 20 m. Without them the astronaut view
looks like velvet. Add them procedurally, with real statistics.

### 6.1 Small craters (PCG + decals/displacement stamps)

Small lunar craters sit in **equilibrium**: cumulative density N(>D) ∝ D⁻², at a few percent of
geometric saturation (Gault 1970; Richardson 2009):

```
N(>D) per m²  ≈  c · 1.54 · D⁻²     (D in metres),   c ≈ 0.01–0.05 (tune; start 0.03)
```

With c = 0.03 that gives ~460 craters ≥ 10 m per km² and ~46,000 ≥ 1 m per km². PCG graph:
*Surface Sampler* on the hero landscape → for size bins (0.5–1, 1–2, 2–5, 5–20 m) sample counts from
the formula → spawn a crater stamp (Nanite mesh with a raised rim + mesh decal blend, or an RVT
height stamp). Depth/diameter ≈ 0.1–0.2 for degraded small craters. Skip bins larger than 20 m,
because the DEM already has those.

### 6.2 Boulders

- Size distribution: cumulative power law N(>D) = K·D⁻ᵇ with steep b (≈ 3–4.6 reported for lunar
  boulder fields, e.g. b = 4.61 from Chandrayaan-2 OHRC at an Apollo 11 crater).
- **Spatial**: boulder abundance **falls off with distance from Shackleton's rim** (ejecta). Boazman
  et al. 2022 mapped 86,896 boulders near Artemis sites 001/004. Drive PCG density by (a) distance
  to mapped crater rims and (b) slope > 15° from the LOLA slope raster.
- Assets: 10–20 scanned Nanite rocks (Fab/Megascans basalt or anorthosite-like), desaturated and
  re-albedoed to 0.10–0.15. Sink them 20–40 % into the ground with small regolith fillets (mesh
  decals), because rocks on the Moon are partly buried.

### 6.3 Tiling breakup

Macro variation from the DEM-derived hillshade/curvature, a mid-scale noise, and micro detail
tiles at 0.5 m and 4 m with a distance blend. Use RVT for the landscape so PCG stamps and decals
blend.

---

## 7. Scale cues (because there is no haze)

Without aerial perspective, people cannot tell a 50 m hill from a 5 km massif. Give them honest
cues:

- A **lander model** (generic, ~10 m) and an **astronaut silhouette** (1.8 m) at the site.
- The horizon overlay's **distance ticks** (1 km, 10 km, 50 km) on the ring.
- A subtle **altitude readout** and a scale bar in the corner.
- A short "Why does it look so close?" ExplainerCard: there is no air, so there is no haze.

---

## 8. The sky

### 8.1 Stars

- **NASA SVS Deep Star Maps 2020**: 1.7 billion stars (Hipparcos-2, Tycho-2, Gaia DR2), OpenEXR
  half-float, plate carrée, **celestial ICRF/J2000** version (use that one for 3D). Pick the 8K or 16K
  resolution.
- Map onto an inverted sphere with an unlit `M_Stars` (Emissive = EXR × StarScale). Rotate it every
  frame by `pxform_c("J2000","MOON_ME",et)` → georeference (chapter 04 §8.1). The constellations
  then sit exactly where they would above the pole. The south celestial pole is almost straight
  overhead.
- StarScale calibrated so stars vanish in photographic mode and appear in dark-adapted mode (§3.4).

### 8.2 Earth

- Textures: NASA Blue Marble Next Generation (monthly, pick the hackathon month: November) for day,
  NASA Black Marble 2016 for night lights, plus a cloud layer.
- **Rotation**: compute the sub-Moon point on Earth with `subpnt_c("NEAR POINT/ELLIPSOID","EARTH",et,"IAU_EARTH","LT+S","MOON",...)`
  and rotate the Earth mesh so that longitude faces the camera. The correct continents then face
  the Moon at that UTC.
- **Phase**: `M_Earth` is **unlit**. It computes day/night from `MPC_Lunar.SunDirWS` (N·L with a soft
  terminator), adds night lights on the dark side and a thin blue Fresnel rim for the atmosphere.
  It must be unlit because terrain shadow maps must not touch a mesh 500 km away.
- Size: true angular diameter 1.82–2.01° from `EarthDistanceKm` (chapter 04 §8.1).
- Partial Earthrise: the terrain occludes the Earth mesh naturally. Correct by construction.

### 8.3 Sun disk

Use the directional light's built-in sun disk (via the sky material) or a small emissive billboard
at 0.533°, with HDR intensity high enough to bloom. No corona, because it is invisible in daylight.

---

## 9. Lumen and bounce light

Near the pole, bounce light from sunlit crater walls is the main thing that lights shadowed
terrain (ShadowCam: earthshine is ~10× dimmer than that secondary light). So GI is a primary
lighting term here.

- Lumen GI on (Epic for stills, High interactive, **Lumen Lite** on laptops).
- PPV → Lumen Global Illumination → raise **Max Trace Distance** and **Scene View Distance** to
  several km so a sunlit wall 2 km away can light a crater floor.
- Enable far-field traces with hardware ray tracing (`r.LumenScene.FarField 1`, VERIFY in 5.8) for
  km-scale bounce.
- Compare with the path tracer on a fixed shot weekly. Lumen should be within "looks the same"
  range.

---

## 10. Cinematic feel

- **Cameras**: CineCameraActor, 24–35 mm for landscapes, 85–135 mm for Earth-over-horizon shots.
  Long lenses compress the bobbing Earth against a ridge, the money shot. f/8–f/11 for deep focus.
- **Camera language**: slow, heavy moves (a tripod on the Moon). No handheld shake except
  first-person astronaut mode at 1/6 g bounce.
- **Movie Render Graph** (Production Ready in 5.8): 3840×2160, Spatial Samples 8–16, Temporal
  Samples 1 (no motion blur), Path Tracer for the hero shots, deferred + Lumen Epic for long
  time-lapses.
- **Time-lapse**: one lunar day (29.5 d) in 20 s. Shadows sweep 360° around every peak. This is the
  single most hypnotic shot you can show.
- **Audio**: space is silent. Use suit ambience (fans, breathing) in astronaut mode, and an
  Apollo-style **"Quindar" beep when Earth rises above the horizon (DTE link acquired)** and a
  lower tone when it sets. It is a sound cue that teaches the comms concept without words.

---

## 11. Realism checklist (judges will notice)

- [ ] Black sky in daylight, no fog anywhere
- [ ] White (not orange) low sunlight
- [ ] Sun elevation and azimuth exactly match the panorama chart at the same UTC
- [ ] Shadow terminator crosses the site when the chart says sunrise
- [ ] Penumbra width consistent with a 0.533° Sun (soft at 10 km, sharp at 10 m)
- [ ] Shadowed floors dark but not pure black near sunlit walls (bounce); earthshine visible in
      eye mode
- [ ] Earth correct size, correct phase (opposite to the Moon's phase seen from Earth), correct
      continents
- [ ] Stars only in dark-adapted mode, with correct constellations
- [ ] Opposition halo around the astronaut's shadow when looking down-Sun
- [ ] No brown or tan regolith, no wet sheen, no repeating tiles visible at 2 m eye height
- [ ] Boulders partly buried, denser near crater rims
- [ ] A side-by-side "LROC photo vs our render" slide for one site

## Sources

- NASA SVS Deep Star Maps 2020: <https://svs.gsfc.nasa.gov/4851>
- DUST (MSC-27522-1): <https://software.nasa.gov/software/MSC-27522-1>
- NASA Langley UE5 lunar lighting testbed: NTRS 20230013102; arXiv 2410.04371
- Hapke regolith parameters for real-time simulation (Chrono::Sensor): <https://www.colorado.edu/ness/sites/default/files/attached-files/efficient_photometric_models_of_the_lunar_regolith_for_virtual_reality_simulation.pdf>
- ShadowCam: <https://www.shadowcam.asu.edu/>
- Apollo photography exposure (f/11, 1/250 s): Apollo Lunar Surface Journal
- Crater equilibrium N ∝ D⁻²: Gault 1970 (*Radio Science*); Richardson 2009 (*Icarus*)
- Boulders near Shackleton: Boazman et al. 2022, *PSJ* (<https://boris.unibe.ch/179288/>)
- UE directional light units (lux) and LightSourceAngle docs: <https://dev.epicgames.com/documentation/en-us/unreal-engine/physical-lighting-units-in-unreal-engine>
