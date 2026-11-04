# 06 — Build Plan: From Today to Submission

> **TL;DR** — Four weeks of prep (toolchain, data, CSPICE, one lit test scene), then a 48-hour sprint
> with hard cut lines. The science is already done and validated (`tools/lunar_sky.py`), so the
> hackathon is about the Unreal integration, the UI and the story. Record the video by Sunday
> afternoon, not Sunday night.

Key dates: **Oct 28 2026**, full challenge statement and NASA data released. **Nov 14–15 2026**,
hackathon. Submit with **"Submit for Judging" before 11:59 p.m. local time** of your Local Event.
Edits after that are impossible.

---

## 1. Rules check (do this first)

- The Project Submission Guide says nothing about preparing before the event (searched Oct 2026).
  **Ask your Local Lead** what may be built in advance. Plan for the strict reading: before
  Nov 14 you may learn, install, download and prototype in a throwaway project, and the submitted
  project is built during the event.
- **AI disclosure is mandatory.** The submission form asks which AI tools you used and how.
  AI-generated images/videos need a **visible watermark**. AI-generated code and data must be
  acknowledged in descriptive text and metadata. AI content must not use or modify NASA branding
  (logos, insignia, mission identifiers). This guide and `tools/lunar_sky.py` were produced with an
  AI assistant, so say so on the project page.
- Global Awards require **NASA data**. List every dataset (chapter 03 §8).
- Demo: **≤ 30 s video with English subtitles** or **≤ 7 slides**, hosted publicly with no login.

---

## 2. Team roles (4–6 people)

| Role | Owns | Hackathon deliverable |
|---|---|---|
| **Science lead** | `LHScience`, CSPICE module, Python tools, validation | C++ results match Python CSV; validation panel |
| **Engine lead** | Cesium, georeference, cameras, subsystems ↔ actors, packaging, Pixel Streaming | Globe → fly-down → surface; packaged build |
| **UI lead** | CommonUI, Panorama/Timeline/Compare charts, Explainers | All widgets live-bound to data |
| **Tech artist** | Landscapes, regolith material, lighting, Earth/stars, PCG rocks | Hero shots that pass the realism checklist |
| **Storyteller / PM** (can be shared) | Pitch, video, project page, time-keeping, judging map | 30-s video, slides, submission |
| (6th) **Data wrangler** | DEM prep, horizon masks, presets, CSV exports | All preset masks + heightmaps |

With 4 people: science + data in one person, storyteller shared by UI and PM.

---

## 3. Pre-hackathon prep (Oct 20 → Nov 13)

### Week 1 (Oct 20–26): toolchain and data
- [ ] Everyone: UE 5.8 + VS 2022 17.14 (Windows) or Xcode 15.4 (Mac); Git + LFS; Cesium for Unreal 2.30.0.
- [ ] Cesium ion account; add **Cesium Moon** to the asset library; note the **asset id**.
- [ ] Science: `pip install spiceypy numpy rasterio`; fetch the kernels (chapter 02 §3); run
      `lunar_sky.py --selftest` and `--validate` (expect ≤ 0.0001° error).
- [ ] Data: download PGDA `ldem_87s_5mpp.tif` (3.3 GB) and site DEMs Site01/04/07/11/20/23 + DM2 +
      Haworth (product 78), plus a coarse polar DEM for far-field horizons.
- [ ] Engine: build `cspice.lib` with `/MD` and `libcspice.a`; keep them on the team drive.

### Week 2 (Oct 27–Nov 2): challenge release and spikes
- [ ] **Oct 28: read the full challenge.** Add any NASA-provided resources/datasets to chapter 03
      and to Sites.json. Check NASA's CLPS page for Griffin-1's landing site.
- [ ] Spike A (throwaway project): CSPICE in UE, with a directional light driven by SPICE at 89.9°S.
      Check the shadow direction against the panorama numbers.
- [ ] Spike B: Cesium Moon + `IAU2015_Moon` georeference at a site; measure the **vertical offset vs
      LOLA** (chapter 04 §10).
- [ ] Spike C: import one 4033² Landscape (Site20 / Mons Mouton); curvature WPO; first regolith pass.
- [ ] Data: `make_horizon_masks.py` for all presets (2 m and 10 m eye heights).

### Week 3 (Nov 3–9): look-dev and UX
- [ ] Lighting recipe from chapter 05 in a test scene; compare against an LROC low-Sun mosaic.
- [ ] UI wireframes in UMG (static data), color tokens, fonts.
- [ ] Pixel Streaming 2 hello-world on one GPU machine.
- [ ] Write the six ExplainerCards and the 30-s storyboard; collect reference images.
- [ ] Compute real metrics for the demo sites with `lunar_sky.py` (IM-2, Malapert, Shackleton ridge
      B, de Gerlache rim). The pitch claims "Malapert keeps Earth in view longer; the ridge gets more
      Sun", so **confirm it with numbers first**.

### Week 4 (Nov 10–13): dry run
- [ ] 4-hour mini-hackathon rehearsal: fresh project, follow chapter 04 §3–§8 end to end.
- [ ] Pack an "event kit": installers, DEMs, kernels, masks, the cspice libs, fonts and reference
      board on an SSD (venue Wi-Fi will fail).
- [ ] Agree on cut lines (§4) and the Git branching rule (main is always demoable).

---

## 4. The 48-hour sprint

Times are hours from kickoff (H0 = Saturday morning, local).

| Hours | Science | Engine | UI | Art | Story/PM |
|---|---|---|---|---|---|
| H0–2 | Repo, modules, CSPICE linked | Project settings, plugins, maps | CommonUI skeleton | Import hero Landscape #1 | Project page draft, judging map |
| H2–8 | `FLHSpice`, `GetSkyState`, unit test vs CSV | Celestial rig (Sun/earthshine/Earth), georeference | Timeline + time subsystem | Lighting recipe, regolith v1 | Storyboard locked, capture plan |
| H8–14 | Horizon CSV loading, disk fraction, DSN | Globe + fly-down, Cesium cut-out | Panorama chart | Earth + stars, PCG rocks v1 | Explainer copy into widgets |
| **H14 checkpoint** | **MVP 1–5 working in editor, ugly is fine** | | | | |
| H14–20 (night) | Analysis sweep async + metrics | Packaged build test #1 (kernels!) | CompareCards | Hero landscapes #2–#3 | Sleep shift A |
| H20–28 | Validation panel, CSV export | Horizon overlay ring + tracks | Site picker polish, tooltips | Realism checklist pass | Record b-roll (MRG) |
| **H28 checkpoint** | **MVP 1–7 done → freeze features. Otherwise cut stretch entirely** | | | | |
| H28–36 | Stretch: runtime masks, mast slider | Astronaut mode, Pixel Streaming | Classroom mode, accessibility | Hero shots for the video | **Edit the video v1 by H34** |
| H36–42 | Bug bash | Packaged build #2 on a clean machine | Copy edit, reduced-motion | Polish | Video v2, subtitles, slides |
| H42–46 | Freeze. Tag `v1.0` | Upload build/zip; PS2 link live | Screenshots | — | Project page complete |
| H46–48 | Buffer | Buffer | Buffer | Buffer | **Submit for Judging** with ≥ 1 h to spare |

Cut order if behind (cut from the top): Pixel Streaming → astronaut mode → runtime masks →
heatmap → Classroom tour → third hero landscape. Never cut: correct Sun/Earth, panorama, timeline,
compare, validation, video.

---

## 5. Risk register

| Risk | Likelihood | Impact | Mitigation / fallback |
|---|---|---|---|
| CSPICE won't link (CRT, arch) | Medium | High | Prebuilt libs from prep; fall back to `Data/Ephemeris/*.csv` from `lunar_sky.py --csv` (interpolate) |
| Kernels missing in packaged build | High | High | Non-UFS staging (chapter 04 §5); packaged test at H14 |
| Cesium Moon datum/coverage issues | Medium | Medium | Local LOLA Landscapes + SVS Moon Kit sphere for the globe |
| Lumen/VSM artifacts at grazing Sun | Medium | Medium | HWRT shadows; path-traced stills for the video |
| GPU too weak on a laptop | Medium | Medium | Lumen Lite + scalability presets; record on the strongest machine |
| Venue Wi-Fi down | High | Medium | Event SSD with everything; no runtime network needs |
| Video late | High | **Critical** | b-roll from H20, edit v1 by H34, owner = storyteller |
| Scope creep | High | High | Cut lines above; PM says no |
| Pixel Streaming flaky | Medium | Low | It's a bonus. Video + zip are the official artifacts |

---

## 6. Testing & validation checklist

- [ ] `lunar_sky.py --selftest` and `--validate` pass (CI or pre-commit).
- [ ] Automation test: C++ vs Python CSV, Sun and Earth el within 1e-4° (chapter 04 §15).
- [ ] Render ↔ math: shadow terminator on the site when SunVisibleFraction ≈ 0.5.
- [ ] Horizon ring overlays the rendered skyline within ~0.2° at 3 preset sites.
- [ ] Illumination for Bussey reference points within ±5–8 points (chapter 03 §6).
- [ ] Packaged Win64 build runs on a machine that never had UE installed.
- [ ] Reduced-motion and keyboard-only navigation work.
- [ ] All numbers show units and the time window.

---

## 7. Submission checklist (Project Submission page)

- [ ] **Project name** + tagline.
- [ ] **High-level summary**: what, how it addresses the challenge, why it matters (3 short paragraphs).
- [ ] **Link to project demo**: YouTube (public/unlisted, no login), ≤ 30 s, English subtitles burned in.
- [ ] **Link to final project**: GitHub repo (README with screenshots) + build download + PS2 link if live.
- [ ] **NASA data used**: LRO LOLA (PGDA 5 m DEMs, polar GDRs), LROC NAC mosaics, NAIF SPICE kernels
      (DE440, lunar PCK/FK), DSN station kernels, NASA SVS Deep Star Maps / CGI Moon Kit, ShadowCam.
- [ ] **Space Agency Partner & other data**: Cesium Moon (Cesium ion), Chandrayaan-3 site (ISRO), etc.
- [ ] **Resources/tools**: Unreal Engine 5.8, Cesium for Unreal, CSPICE/spiceypy, GDAL, rasterio.
- [ ] **AI disclosure**: which tools, for what. Watermark any AI images/video. No NASA logos in AI content.
- [ ] Team members listed; project page lock off; link tested in a private browser window.
- [ ] Click **Submit for Judging** (not just Save).

---

## 8. Judge's-eye polish checklist (last 4 hours)

- [ ] First 5 seconds of the app are beautiful (spinning Moon, real stars).
- [ ] No placeholder text, no "Lorem", no debug prints on screen.
- [ ] Every chart has a title, axis labels and a legend.
- [ ] Validation badge visible on the main HUD.
- [ ] Credits screen with NASA data sources.
- [ ] The killer facts (chapter 01 §11.4) appear somewhere in the UI or Explainers.
- [ ] One "real vs render" comparison in the slides/README.
- [ ] Build starts in < 20 s and never shows a black screen (loading screen with a tip).

---

## 9. Open questions to resolve (all "VERIFY" items)

| # | Question | Where to check | Chapter |
|---|---|---|---|
| 1 | Cesium Moon asset id; vertical offset vs LOLA (~2.5 km reported) | Cesium ion Asset Depot; sample a known peak | 03 §1, 04 §10 |
| 2 | Hero Landscape rotation sign (−λ) at the site | Visual match with Cesium tiles | 04 §10.1 |
| 3 | Mac target architectures for the CSPICE universal lib | UE 5.8 Mac platform settings | 04 §4.1 |
| 4 | Pixel Streaming 2 command-line flags and infrastructure branch for 5.8 | Epic PS2 docs, PixelStreamingInfrastructure repo | 04 §14 |
| 5 | Cloud GPU pricing for PS2 | AWS/Azure pricing pages | 04 §14 |
| 6 | Exact LOLA polar GDR file names for far-field DEMs | PDS Geosciences directory listing | 03 §1 |
| 7 | LOLA 1064 nm normal-albedo polar product location | PGDA / PDS | 03 §1, 05 §5 |
| 8 | Griffin-1 and Blue Moon MK1 landing coordinates | NASA CLPS page; Oct 28 challenge resources | 03 §2.2 |
| 9 | Slater Plain center (map-derived); IM-1 decimals | NASA region map; LROC IM-1 page | 03 §2 |
| 10 | Artemis II Earthset image id | images.nasa.gov | 05 §1 |
| 11 | VSM clipmap LastLevel default; Nanite Tessellation status; `r.LumenScene.FarField` in 5.8 | Console (`help r.Shadow.Virtual.Clipmap.LastLevel`), 5.8 release notes | 05 §4, §5, §9 |
| 12 | Pre-hackathon work rules | Local Lead | 06 §1 |
| 13 | Demo claims (Malapert vs ridge) | Run `lunar_sky.py` with masks | 06 §3 week 3 |
