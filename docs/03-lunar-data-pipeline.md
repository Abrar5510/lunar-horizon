# 03 — Lunar Data Pipeline

> **TL;DR** — Use real NASA terrain everywhere. The LOLA 5 m/px south-pole mosaic and the PGDA
> per-site 5 m DEMs give hero terrain and near-field horizons. The coarser LOLA polar grids give
> far-field horizons. LROC NAC low-Sun mosaics are detail and reference imagery. The NASA SVS CGI
> Moon Kit (or Cesium Moon Terrain) is the globe. GDAL turns it all into Unreal heightmaps plus a
> float32 "science copy" for horizon masks. Every preset site has a published source, and several
> have published illumination numbers we validate against.

This chapter is the **Relevance** pillar. The rubric asks whether the project "meaningfully
integrates NASA data, utilizing it as a core component … rather than a superficial addition".
Every pixel of terrain and every number in this app comes from LRO.

---

## 1. Dataset catalogue

| Dataset | What / resolution | Format & projection | Size | Use in app | Where |
|---|---|---|---|---|---|
| **LOLA 5 m/px south-pole mosaic** (87.5°S → pole) | Surface height, 5 m/px, 30,336 × 30,336 px (~152 × 152 km) | GeoTIFF `ldem_87s_5mpp.tif` (float, m); also PDS IMG: 16-bit LSB int, ×0.5 m, offset 1,737,400 m. South polar stereographic, center −90°/0°, R = 1737.4 km, MOON_ME (DE421), pixel-registered | 3.3 GB (+ slope 4.9 GB, counts 140 MB, hillshade 820 MB) | Hero terrain + near-field horizons for sites poleward of 87.5°S | `https://pgda.gsfc.nasa.gov/data/LOLA_5mpp/87S/` |
| **PGDA per-site 5 m DEMs** (Barker et al. 2021) | ~16 × 16 km regions: Site01 Connecting ridge, Site04 Shackleton rim, Site06 Nobile rim 1, Site07 Peak near Shackleton, Site11 de Gerlache rim, Site20 Leibnitz β plateau (= Mons Mouton), Site23 Malapert massif, Site42 de Gerlache–Kocher massif, Haworth, Shoemaker, DM1 Amundsen rim, DM2 Nobile rim 2, SL2 de Gerlache rim, SL3 Connecting ridge ext., NPA–NPD, LM1–LM8 | GeoTIFF, south polar stereographic (m), MOON_ME/DE421. `SiteXX_final_adj_5mpp_surf.tif` (height), `_slp.tif`, `_toterr.tif` (RMS error), `_ldec.tif` (returns/pixel), 100 error clones, ROI shapefiles | 10s–100s MB per site | Hero terrain for sites north of 87.5°S (Mons Mouton, Malapert, Nobile, Haworth). Error maps power an "uncertainty" overlay | `https://pgda.gsfc.nasa.gov/products/78` |
| **LOLA polar GDR grids** | 10–240 m/px polar stereographic DEMs at several latitude extents (e.g. `LDEM_87S_5M`, coarser 20/40/60/120/240 m products) | PDS IMG + LBL (16-bit) and JPEG2000 | MB–GB | Far-field horizons (10–150 km), regional context mesh | `https://pds-geosciences.wustl.edu/lro/lro-l-lola-3-rdr-v1/lrolol_1xxx/data/lola_gdr/polar/` (VERIFY: pick the exact file names from the directory listing) |
| **LROC NAC low-Sun controlled mosaics** | 1 / 5 / 20 m/px regional mosaics, binned by sub-solar longitude (A–F). E.g. `NAC_ROI_MTMOUTONLO1_P854S0353` (Mons Mouton / Nobile Rim 1, 84.8–86.1°S) | GeoTIFF / PDS | 1 m mosaic ≈ 1.4 GB | Reference imagery, side-by-side "real vs rendered", detail breakup. **Not** albedo, because shadows are baked in | `https://data.lroc.im-ldi.com/lroc/rdr_product_select` (search "low-Sun") |
| **LOLA 1064 nm normal albedo** (polar) | Shadow-free reflectance measured by the laser itself, works inside PSRs | GeoTIFF/IMG polar stereographic | small | Base albedo variation for the regolith material (chapter 05) | PGDA / PDS LOLA (VERIFY: locate the "LOLA normal albedo" polar product, Lemelin et al. 2016) |
| **ShadowCam** (KPLO) | Imagery *inside* permanently shadowed regions | PDS | — | Explainer cards ("what's in the dark"), PSR texture reference | `https://www.shadowcam.asu.edu/` |
| **NASA SVS CGI Moon Kit** | Global LROC WAC color + LOLA displacement maps (multiple resolutions) | TIFF/PNG equirectangular | up to GBs | Globe view in the site picker | `https://svs.gsfc.nasa.gov/4720` |
| **Cesium Moon Terrain** (Cesium ion) | Global streamed 3D tiles of the Moon | 3D Tiles | streamed | Alternative globe/regional layer inside Unreal (chapter 04) | Cesium ion Asset Depot. Asset id: VERIFY in your account (a forum post cites 2684829) |
| **PGDA illumination studies** | Published average illumination at named points/sites | Tables/papers | — | **Validation targets** (§6) | `https://pgda.gsfc.nasa.gov/products/69`, `/products/78` |

**Datum consistency:** LOLA heights are relative to a 1737.4 km sphere, in MOON_ME. That is the same
sphere and frame the science engine uses (chapter 02 §3–4). A DEM height of −2,000 m means radius
1735.4 km. No conversion is needed.

**Accuracy to quote:** PGDA 5 m site DEMs have median RMS height error ≈ 0.30–0.50 m and slope error
≈ 1.5–2.5°. About 90 % of 5 m pixels are interpolated between LOLA tracks. They are shadow-free
(laser), so they are better for illumination than photo-stereo DEMs. Put this in the
ExplainerCard: it is a credibility signal judges like.

---

## 2. Preset sites (`Content/Data/Sites.json`)

Coordinates are MOON_ME, east-positive longitude (west longitudes converted: 291.2°E = 68.8°W).

### 2.1 NASA Artemis candidate regions (nine, announced Oct 28 2024)

The Artemis III flight was restructured in Feb 2026 into a 2027 low-Earth-orbit lander test.
**Artemis IV (target 2028)** is now the first crewed landing near the south pole, and NASA's
mission page still refers to the nine candidate regions. Each region is ~15 × 15 km and holds
landing sites of ~100 m radius. Centers below come from the LROC NAC low-Sun mosaic product
centers for each region. Where no product exists, the center was read off NASA's region map, so
treat those as ±10 km.

| Id | Region | Lat | Lon (E) | Confidence / source |
|---|---|---|---|---|
| `ART_CABEUSB` | Peak near Cabeus B | −83.6 | 291.2 | LROC `NAC_ROI_CABEUSPKLO1_P836S2912` |
| `ART_HAWORTH` | Haworth | −86.7 | 336.3 | LROC `NAC_ROI_HAWORTH_LO1_P867S3363` |
| `ART_MALAPERT` | Malapert Massif | −86.0 | 0.3 | LROC `NAC_ROI_MALAPERTLO1_P860S0003` |
| `ART_MOUTONPLT` | Mons Mouton Plateau | −84.3 | 30.6 | LROC `NAC_ROI_MOUTNPLTLO1_P843S0306` |
| `ART_MOUTON` | Mons Mouton | −85.5 | 31.9 | map-derived (±10 km) |
| `ART_NOBILE1` | Nobile Rim 1 | −85.5 | 37.8 | map-derived; LROC combined product `NAC_ROI_MTMOUTONLO1_P854S0353` centered −85.4/35.3 |
| `ART_NOBILE2` | Nobile Rim 2 | −83.9 | 58.7 | LROC `NAC_ROI_NOBILE02LO1_P839S0587` |
| `ART_DEGERLACHE2` | de Gerlache Rim 2 | −88.5 | 293.3 | LROC `NAC_ROI_DEGERRIMLO1_P885S2933` (de Gerlache Rim) |
| `ART_SLATER` | Slater Plain | −87.2 | 125.2 | map-derived (±10 km), VERIFY |

When NASA publishes region polygons, replace these centers. The app should treat a region as a
15 × 15 km box and let the user drop a site anywhere inside it.

### 2.2 Landed and planned missions (CLPS and international)

| Id | Mission | Lat | Lon (E) | Status (as of Oct 2026) | Source |
|---|---|---|---|---|---|
| `CLPS_IM2` | Intuitive Machines IM-2 *Athena* (actual resting point, on its side in a 20 m crater, Mons Mouton) | −84.7906 | 29.1957 | Landed Mar 6 2025, mission ended early | LROC images 1401/1408 |
| `CLPS_IM2_TGT` | IM-2 intended site | −84.78 | 29.13 | — | LROC |
| `CLPS_IM1` | Intuitive Machines IM-1 *Odysseus* (near Malapert A) | −80.13 | 1.44 | Landed Feb 2024 | LROC (VERIFY decimals) |
| `CLPS_GRIFFIN1` | Astrobotic Griffin-1 (Falcon Heavy) | VERIFY | VERIFY | Launch targeted late 2026, the only CLPS launch expected in 2026; environmental tests finished Oct 13 2026 | NASA CLPS updates (Sep–Oct 2026) |
| `CLPS_BLUEMOON_MK1` | Blue Origin Blue Moon MK1, will deliver **VIPER** | VERIFY | VERIFY | First part of 2027 | NASA timeline (Sep 15 2026) |
| `INT_CH3` | Chandrayaan-3 *Shiv Shakti* point | −69.373 | 32.319 | Landed Aug 23 2023 | ISRO / Wikipedia |
| `INT_CE7` | Chang'e-7 target (peak on SE ridge of Shackleton) | −88.8 | 123.4 | Launch postponed Aug 23 2026; a single source suggests Feb–Mar 2027 | Planetary Society; Wikipedia |

Griffin-1 lands during or right after the hackathon. That makes it the perfect "live" preset if its
site is published by then. Check NASA's CLPS page on Oct 28 when the challenge data drops.

### 2.3 Science reference points (with published illumination, used for validation)

From Bussey et al. 2010 (*Icarus* 208), tabulated on PGDA product 69. Values are the fraction of a
lunar year illuminated:

| Id | Point | Lat | Lon (E) | Published illumination (yr / min day / max day) |
|---|---|---|---|---|
| `REF_A` | Shackleton crater ridge, point A | −89.68 | 194.0 | 81 % / 44 % / 98 % |
| `REF_B` | Peak near Shackleton ridge, point B | −89.44 | 218.2 | 82 % / 56 % / 100 % |
| `REF_C` | de Gerlache crater ridge, point C | −88.71 | 291.3 | 85 % / 64 % / 98 % |
| `REF_D` | Peak close to Shackleton ridge, point D | −88.79 | 124.5 | 86 % / 58 % / 100 % |
| `REF_M1` | Malapert Mountain M1 | −86.04 | 2.7 | 74 % / 54 % / 95 % |
| `REF_POLE` | Geographic pole (validation site) | −89.9 | 0.0 | — (Horizons cross-check, chapter 02 §9) |

### 2.4 Schema

`FLHSite` in C++ maps 1:1 to this JSON. Load it with `FJsonObjectConverter::JsonArrayStringToUStruct`.

```json
[
  {
    "Id": "CLPS_IM2",
    "Name": "IM-2 Athena (Mons Mouton)",
    "LatDeg": -84.7906,
    "LonDeg": 29.1957,
    "HeightM": 0,
    "Kind": "CLPSMission",
    "Region": "Mons Mouton",
    "Source": "LROC https://lroc.im-ldi.com/images/1408",
    "Dem": "Site20_final_adj_5mpp_surf.tif",
    "Blurb": "Landed 6 Mar 2025, tipped over in a 20 m crater; operated about a day."
  },
  {
    "Id": "ART_MALAPERT",
    "Name": "Malapert Massif",
    "LatDeg": -86.0, "LonDeg": 0.3, "HeightM": 0,
    "Kind": "ArtemisRegion",
    "Region": "Malapert Massif",
    "Source": "LROC NAC_ROI_MALAPERTLO1_P860S0003",
    "Dem": "Site23_final_adj_5mpp_surf.tif",
    "Blurb": "A 5 km-high massif that keeps Earth in view more often than the pole itself."
  }
]
```

`HeightM = 0` means "sample the DEM at load time". `tools/prep_dem.py` fills it in, so the science
engine always uses the true surface radius.

---

## 3. Projection math (needed for every tool)

LOLA polar stereographic (south), sphere R = 1,737,400 m, true scale at the pole:

```
ρ = 2R · tan(π/4 + φ/2)          φ = latitude (negative), λ = east longitude
x = ρ · sin λ
y = ρ · cos λ
```

PROJ string (use with GDAL; it matches the LOLA/PGDA products):

```
+proj=stere +lat_0=-90 +lat_ts=-90 +lon_0=0 +k=1 +x_0=0 +y_0=0 +R=1737400 +units=m +no_defs
```

Check a site:

```bash
echo "29.1957 -84.7906" | gdaltransform \
  -s_srs "+proj=longlat +R=1737400 +no_defs" \
  -t_srs "+proj=stere +lat_0=-90 +lat_ts=-90 +lon_0=0 +k=1 +x_0=0 +y_0=0 +R=1737400 +units=m +no_defs"
```

Sanity numbers: 1° of latitude near the pole ≈ 30.3 km. The 87.5°S mosaic edge is ρ ≈ 75.8 km.

Grid "up" is **not** north (chapter 02 §6.2). Near the pole the meridians converge, so the
direction of north in the grid depends on longitude. The horizon marcher and the Unreal
georeference both have to respect this.

---

## 4. GDAL recipes

Install: `brew install gdal` (macOS) or OSGeo4W / `conda install -c conda-forge gdal rasterio`
(Windows). Python bindings: `pip install rasterio numpy`.

### 4.1 Inspect

```bash
gdalinfo -stats ldem_87s_5mpp.tif | head -40
```

Expect min ≈ −8,760 m and max ≈ +4,744 m for the 87.5°S mosaic (label DERIVED_MIN/MAX). That is a
13.5 km relief inside 75 km of the pole, more than Everest from sea level.

### 4.2 Cut a hero tile around a site (e.g. 20 × 20 km)

```bash
# site center in stereographic metres (from gdaltransform above)
CX=-10501; CY=-13345       # REF_B, peak near Shackleton (89.44°S, 218.2°E): inside the 87.5°S mosaic
H=10000                    # half-size in metres
gdal_translate -projwin $((CX-H)) $((CY+H)) $((CX+H)) $((CY-H)) \
  ldem_87s_5mpp.tif site_hero_5m.tif
# IM-2 (84.79°S, 29.20°E) is at x=77108, y=137993: outside the mosaic (ρ = 158 km > 75.8 km),
# and also outside PGDA Site20 (x 65–81 km, y 110–126 km).
```

> **As built:** `tools/prep_dem.py` does not cut from one file. It composites every DEM in the
> folder that overlaps the site (coarse first, finer on top), so IM-2 gets the 20 m GDR and Malapert
> gets PGDA Site23 at 5 m with no per-site configuration. The PDS `*_float.img` GDRs store
> **kilometres**, not metres (label `UNIT = KILOMETER`); the tool scales them ×1000.

### 4.3 Science copy (float32, never quantized)

```bash
gdal_translate -ot Float32 site_hero_5m.tif site_hero_5m_f32.tif
# far field: 150 km radius at coarser resolution for horizon masks
gdalwarp -te $((CX-150000)) $((CY-150000)) $((CX+150000)) $((CY+150000)) \
  -tr 40 40 -r average <coarse_polar_dem> site_far_40m_f32.tif
```

### 4.4 Unreal Landscape heightmap (16-bit PNG)

Unreal maps a 16-bit heightmap to ±256 m at Z-scale 100 (1 unit = 1/128 cm × scale). For a tile with
heights from `Hmin` to `Hmax` (metres):

```
ZScale (%)   = (Hmax − Hmin) / 512 × 100  = (Hmax − Hmin) × 0.1953125
XY scale (cm) = pixel size in cm        (5 m → 500; 2 m resample → 200)
Landscape Z  = (Hmin + Hmax) / 2 × 100 cm   (place the actor so 32768 sits at the mid height)
```

```bash
# resample to an Unreal-friendly size (e.g. 4033 × 4033 px at 5 m ≈ 20.2 km)
gdalwarp -ts 4033 4033 -r bilinear site_hero_5m_f32.tif site_hero_4033.tif
gdal_translate -ot UInt16 -scale $HMIN $HMAX 0 65535 -of PNG site_hero_4033.tif site_hero_4033.png
```

Unreal's recommended landscape sizes are of the form (quads per section × sections per component ×
components) + 1, e.g. 1009, 2017, 4033, 8129. Pick one, or the editor pads or crops. With a 13.5 km
relief, 16 bits give ~0.2 m steps. That is fine for rendering. **Science never reads the PNG.**

Chapter 04 §6 decides between Landscape, Nanite static mesh and Cesium tiles. The same float32
tile feeds all three.

### 4.5 Normal + detail maps

```bash
gdaldem hillshade site_hero_5m_f32.tif hillshade.tif -az 0 -alt 1.5   # preview "pole light"
gdaldem slope site_hero_5m_f32.tif slope.tif                          # landing-hazard overlay
```

Use the slope raster as a "landing hazard" heatmap. Show slopes above 8–10° in red, the typical
lander limit.

---

## 5. The two offline tools

### 5.1 `tools/prep_dem.py`

Input: `Sites.json` + DEM paths. Output, per site: the hero tile (float32 + 16-bit PNG + JSON
sidecar with ZScale/XY scale/actor Z), the far-field float32 tile, and `HeightM` sampled at the site
center written back to `Sites.json`. It just wraps the GDAL commands above via
`subprocess`/`rasterio`.

### 5.2 `tools/make_horizon_masks.py`

```python
"""DEM tiles -> Content/Data/Horizons/<SiteId>.csv (0.25 deg bins)."""
import json, sys, numpy as np, rasterio
from lunar_sky import horizon_mask

def mask_for(path, x, y, lon, max_km, eye_m):
    with rasterio.open(path) as ds:
        dem = ds.read(1).astype(np.float32)
        row, col = ds.index(x, y)
        px = ds.res[0]
    return horizon_mask(dem, px, row, col, lon, step_deg=0.25, max_km=max_km, eye_m=eye_m)

def main(sites_json, out_dir, eye_m=2.0):
    for s in json.load(open(sites_json)):
        x, y = stereo_xy(s["LatDeg"], s["LonDeg"])                 # §3 formula
        near = mask_for(s["HeroTile"], x, y, s["LonDeg"], 12, eye_m)   # 5 m, first ~12 km
        far = mask_for(s["FarTile"], x, y, s["LonDeg"], 150, eye_m)    # 40 m, out to 150 km
        m = np.maximum(near, far)                                   # union of both horizons
        az = np.arange(len(m)) * 0.25
        np.savetxt(f"{out_dir}/{s['Id']}.csv", np.c_[az, m], delimiter=",",
                   header="azimuth_deg,elevation_deg", comments="", fmt="%.2f,%.3f")

def stereo_xy(lat, lon, R=1737400.0):
    import math
    rho = 2 * R * math.tan(math.pi / 4 + math.radians(lat) / 2)
    return rho * math.sin(math.radians(lon)), rho * math.cos(math.radians(lon))

if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2])
```

Run time: ~1–3 s per site in NumPy. All presets take under a minute. Commit the CSVs, because they
are the fallback if anything else breaks.

**Eye height** is a parameter (default 2 m). Also export `_10m` variants for a "mast" toggle. A tall
mast gains a lot of illumination and Earth visibility near the pole, and that comparison is a
compelling demo.

---

## 6. Validation

| Check | How | Pass |
|---|---|---|
| Heights | Sample `ldem_87s_5mpp.tif` at REF points, compare with PGDA/LOLA values or the hillshade | Visually coherent, no sign flips (craters negative) |
| Projection | Round-trip lat/lon → x/y → lat/lon with `gdaltransform` | < 1 m |
| Horizon | Self-test (chapter 02 §9.3) plus visual: render a 360° panorama in Unreal from the site and overlay the mask CSV | Skyline matches within ~0.2° |
| Illumination | Run `lunar_sky.py --site <REF> --mask <csv> --days 365` over a lunar year and compare with §2.3 | Within ±5–8 points of Bussey 2010 (different DEMs, eras and resolution) |
| Uncertainty | Run masks on 3–5 PGDA error clones, report the spread of `IlluminationPct` | Show as "± x %" in CompareCards |

The uncertainty band from clones is a feature that turns a hackathon tool into something a mission
planner would trust. It costs one loop.

**Result (2026-10-21, 2 m eye, 2026 sweep at 2 h steps, PGDA 5 m + LOLA 60 m GDR to 150 km).** At
Bussey's printed coordinates every point sits on a 5–16° slope and scores only 19–50 %. Those
coordinates come from Kaguya-era topography and are given to 0.01° in latitude (~300 m). The
best-lit peak within 2 km of each one matches the published value:

| Point | Published | Best-lit peak within 2 km (ours) | Moved |
|---|---|---|---|
| A, Shackleton ridge | 81 % | 85.4 % | 593 m |
| B, peak near Shackleton | 82 % | 88.4 % | 1,379 m |
| C, de Gerlache ridge | 85 % | 83.4 % | 1,292 m |
| D, peak close to Shackleton | 86 % | 84.8 % | 753 m |
| M1, Malapert | 74 % | 80.6 % | 1,914 m |

All within ±7 points. `Sites.json` now holds the relocated peaks (each `Source` keeps Bussey's original
coordinates). Show this table in the validation panel: it is honest, and it shows why 5 m terrain matters.

---

## 7. Storage budget

| Item | Size |
|---|---|
| SPICE kernels | ~50 MB (shipped) |
| Horizon CSVs (25 sites × 2 eye heights) | < 2 MB (shipped) |
| Sites.json, fallback ephemeris CSVs | < 20 MB (shipped) |
| Hero heightmaps (4033² 16-bit PNG ×6) | ~200 MB (shipped as Unreal assets, compressed) |
| Raw DEMs, mosaics (work only) | 10–20 GB (**not** in the repo; use a shared drive or `tools/fetch_data.sh`) |

Add the raw data folder to `.gitignore`. Use Git LFS only for the Unreal `.uasset` heightmaps.

---

## 8. Attribution and licensing

- LRO/LOLA/LROC data are NASA products and free to use. Credit "NASA/GSFC/MIT (LOLA)" and
  "NASA/GSFC/Arizona State University (LROC)".
- Cite Barker et al. 2021, *PSS* 203, 105119 for the 5 m DEMs (PGDA asks for it).
- Cite Mazarico et al. 2011 (*Icarus* 211) and Bussey et al. 2010 (*Icarus* 208) for illumination
  comparisons.
- NASA SVS imagery: credit "NASA's Scientific Visualization Studio".
- Cesium ion content has its own terms. Read them before redistributing tiles.
- Space Apps Global Awards **require** NASA data. List every dataset above on the project page.

## Sources

- PGDA LOLA 5 m 87°S mosaic: <https://pgda.gsfc.nasa.gov/data/LOLA_5mpp/87S/>
- PGDA site DEMs (product 78) and illumination table (product 69): <https://pgda.gsfc.nasa.gov/products/78>, <https://pgda.gsfc.nasa.gov/products/69>
- LROC RDR product catalog: <https://data.lroc.im-ldi.com/lroc/rdr_product_select>
- IM-2 resting site: <https://lroc.im-ldi.com/images/1408>
- Artemis candidate regions (Oct 2024, 9 regions; Aug 2022, 13 regions): NASA news releases RELEASE22-089 and Oct 28 2024 update
- Artemis restructuring (Feb 2026): NASA Artemis mission pages; contemporary coverage
- NASA SVS CGI Moon Kit: <https://svs.gsfc.nasa.gov/4720>
