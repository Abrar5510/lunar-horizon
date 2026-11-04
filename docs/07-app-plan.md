# 07 — App Plan: Where We Are and What to Build Next

*Status snapshot 2026-10-28 (challenge release day). Chapters 01–06 are the design; this page is
the working plan that maps that design to what exists on disk today.*

## 1. Blocker first: the Mac can't build Unreal yet

| Needed | State on this Mac | Action (you) |
|---|---|---|
| Unreal Engine 5.8 | **Not installed** (only `installers/EpicGamesLauncher.dmg`) | Install the launcher, sign in, install UE 5.8 **to the Transend SSD** (internal disk has 13 GB free; UE needs ~60 GB) |
| Xcode 15.4+ | **Not installed** (Command Line Tools only) | Install from the App Store, then `sudo xcode-select -s /Applications/Xcode.app` |
| Cesium for Unreal 2.30.0 | Zip + unpacked plugin in `plugins/` | Copy into `LunarHorizon/Plugins/` or the engine's Marketplace folder after UE is in |
| Cesium ion account + Moon asset | Unknown | Free non-commercial account; add Cesium Moon Terrain, note the asset id |

Everything below that says *editor* waits on this row. Everything else can move now.

## 2. MVP features vs. what exists

| # | MVP feature (01 §6) | Code that exists | Still to make | Who |
|---|---|---|---|---|
| 1 | Site picker on 3D globe | `Sites.json` (20 sites), `ULHEphemerisSubsystem` | `L_Globe` map with Cesium Moon, site pins, `WBP_SitePicker` | Engine + UI |
| 2 | Fly-down to surface | — | Camera spline / `CesiumFlyToComponent`, hero Landscape import per site | Engine |
| 3 | Correct Sun & Earth | `ALHCelestialRig`, `LHSpice` (C++ matches Python to 5e-6°) | `BP_CelestialRig` with lights, Earth mesh + material, star dome | Engine + Art |
| 4 | Horizon panorama | `ULHHorizonPanoramaWidget` (NativePaint), 40 horizon CSVs | `WBP_HorizonPanorama` layout and binding | UI |
| 5 | Timeline + speeds + Gantt | `ULHTimeSubsystem`, `ULHTimelineWidget` | `WBP_Timeline`, play/pause/speed buttons | UI |
| 6 | CompareCards | `ULHAnalysisSubsystem` sweep → `FLHSiteMetrics` | `WBP_CompareCards` | UI |
| 7 | Validation badge | Validation numbers from `lunar_sky.py --validate` | `WBP_Validation` panel with the table | UI |

**Art (no code):** `M_Regolith` master material, hero Landscapes (start with `ART_MOUTON`,
`CLPS_IM2`, `ART_MALAPERT`), exposure / post-process volume per chapter 05.

## 3. Order of work

**Now, no Unreal needed**
1. Keep the data current: re-run `tools/fetch_and_prep.sh` after any tool or `Sites.json` change,
   then `tools/cpp_check/run.sh <site ids>`.
2. Read the 2026 challenge statements released today; add any NASA-supplied datasets and update
   the challenge wording in chapter 01 if our challenge was renamed or reshaped.
3. Compute the demo claims (§9 item 13) from the masked metrics and write them into 01 §11.
4. UI wireframes on paper/Figma using the real metric numbers.

**Day 1 after UE + Xcode are in (spikes from 06 §3 week 2)**
1. Open `LunarHorizon.uproject` → let it compile both modules. Fix errors (first compile ever).
2. Run the `LHScience` automation test in Session Frontend.
3. Spike A: `L_Surface_Test` with a Landscape + `ALHCelestialRig` at `REF_POLE`; check shadow
   direction against the panorama.
4. Spike B: Cesium Moon at a site; measure the vertical offset vs. LOLA (reported ~2.5 km).
5. Spike C: import `sites/ART_MOUTON_hero.png` as a 4033² Landscape using its `_hero.json`.

**Week 3 (Nov 3–9)**: lighting look-dev, real widgets bound to subsystems, packaged build #0
(proves kernels ship). **Week 4**: 4-hour rehearsal, event kit on the SSD.

## 4. Maps and assets to create (editor)

```
Content/
  Maps/       L_Globe, L_Surface (one map, hero Landscape streamed per site), L_Test
  Blueprints/ BP_CelestialRig, BP_SitePin, BP_FlyDownCamera, BP_LHGameMode
  UI/         WBP_Main, WBP_SitePicker, WBP_HorizonPanorama, WBP_Timeline,
              WBP_CompareCards, WBP_Validation, WBP_ExplainerCard
  Materials/  M_Regolith (+ MI per site), M_Earth, M_StarDome, M_HorizonOverlay
  Data/       Sites.json, Horizons/*.csv   (exists)
  SPICE/      kernels                      (exists)
```

## 5. Open items (06 §9) — status after 2026-10-28

| # | Item | Status |
|---|---|---|
| 1 | Cesium Moon asset id / LOLA offset | Forum cites **2684829** and a ~2.5 km offset vs LOLA; still confirm in our account (Spike B) |
| 2 | Hero Landscape rotation sign | Needs editor (Spike B/C) |
| 3 | Mac CSPICE architectures | Universal `libcspice.a` links and runs on this Mac (`cpp_check` passes); confirm UE target arch at first compile |
| 4 | Pixel Streaming 2 flags for 5.8 | Not published in searchable docs; check PS2 Overview + infrastructure repo branches once UE is in. Stretch item only |
| 5 | Cloud GPU pricing | Deferred: PS2 is first on the cut list |
| 6 | LOLA polar GDR file names | **Resolved**: `ldem_60s_60m`, `ldem_75s_60m`, `ldem_80s_20m` float IMG/LBL are downloaded and used |
| 7 | LOLA 1064 nm albedo product | Open (art nice-to-have) |
| 8 | Griffin-1 / Blue Moon MK1 coordinates | No published landing point. Griffin-1 targets **Mons Mouton (84.6°S, 31.0°E)** near Nobile, launch NET Nov 2026; added as provisional `CLPS_GRIFFIN1`. Blue Moon MK1 targets Shackleton Connecting Ridge, no coordinates or date |
| 9 | IM-1 decimals / Slater Plain | IM-1 **confirmed** 80.13°S 1.44°E, 2579 m (LROC; our DEM gives ~2595 m). Slater still map-derived |
| 10 | Artemis II Earthset image id | Open |
| 11 | VSM / Nanite / Lumen far-field cvars | Needs editor |
| 12 | Pre-hackathon work rules | **Ask your Local Lead** — decides whether this repo can be the submitted project |
| 13 | Demo claims | **Checked** (§6): ridge-more-Sun holds, Malapert-more-Earth does not |

## 6. Demo claims (item 13)

From `lunar_sky.py` with the 2 m horizon masks, 2026-11-14 + 30 days (run 2026-10-29).

| Site | Sunlit % | Longest dark (h) | Earth above horizon (DTE) % | DSN % | Avg power (W) |
|---|---|---|---|---|---|
| CLPS_IM2 | 51.6 | 311.5 | 81.2 | 98.0 | 214.7 |
| CLPS_GRIFFIN1 (provisional, 20 m DEM only) | 40.8 | 341.5 | 49.8 | 98.0 | 169.5 |
| ART_MALAPERT | 51.1 | 211.0 | 31.7 | 98.0 | 215.2 |
| ART_MOUTON | 54.3 | 322.5 | 0.0 | 98.0 | 228.6 |
| ART_DEGERLACHE2 | 43.2 | 339.5 | 49.6 | 98.0 | 180.1 |
| ART_HAWORTH | 34.6 | 315.5 | 18.8 | 98.0 | 145.2 |
| REF_A (near Shackleton) | 100.0 | 0.0 | 43.1 | 98.0 | 419.5 |
| REF_B (connecting ridge area) | 100.0 | 0.0 | 46.5 | 98.0 | 419.5 |

**What this means for the pitch (01 §11):**
- "The ridge gets more Sun": **true** for this window (100 % vs ~51 % at Malapert).
- "Malapert keeps Earth in view longer": **false** for this window (32 % vs 47 % at REF_B).
  Rewrite the line. IM-2 is the Earth-view standout at 81 %.
- ART_MOUTON shows 0 % DTE. That's real, not a bug: the region point is ~100 km south of the
  Mons Mouton summit, and the massif blocks Earth (horizon 11–14° toward az 0°; IM-2's is ~0°).
  That makes a strong demo moment.
- One 30-day window only. Re-run over a full year before quoting percentages as typical.
