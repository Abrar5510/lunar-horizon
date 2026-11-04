# 01 — Challenge, Product & UX: What We Build and Why It Wins

> **TL;DR** — The 2026 challenge **"CLPS Lunar Mission Browser"** asks for an *intuitive* way to
> compare south-pole landing sites and dates by Sun/Earth position versus the horizon, for power
> and direct-to-Earth (DTE) comms. Existing tools are accurate but are text tables, code libraries
> or static maps. We build a photoreal, time-scrubbable 3D Moon where you pick a site, fly down,
> and *see* the Sun skim the ridgeline and the Earth bob in and out of view. Every pixel is backed
> by NASA data and validated against JPL Horizons. Three audiences, one app: mission planners get
> numbers, educators get explanations, the public gets goosebumps.

---

## 1. The challenge, decoded

**Official title:** *CLPS Lunar Mission Browser* (Space Apps 2026,
`spaceappschallenge.org/2026/challenges/clps-lunar-mission-browser/`). Full statement and NASA data
links publish **Oct 28 2026**. The hackathon is **Nov 14–15 2026**.

The statement, verb by verb:

| Phrase in the statement | What the judges will look for | Our answer |
|---|---|---|
| "extreme lighting and communication constraints" | You understand *why* the pole is hard | Live demo of a 1.5° Sun and Earth dipping below the horizon |
| "Tools … exist, but their complicated interfaces …" | You beat Horizons/SPICE/GIS **on usability**, not on accuracy | Click a site → answers in < 5 s |
| "mission planners, educators, and the public" | Three audiences served without three apps | One app, three depths: KPI cards, Explainer cards, cinematic mode |
| "compare landing sites and dates quickly" | Side-by-side comparison plus fast date scrubbing | CompareCards plus a Timeline scrubber |
| "visualizing Sun and Earth positions relative to the horizon" | Horizon-relative visuals, not just numbers | HorizonPanorama plus a 3D horizon overlay |
| "power generation potential" | Illumination → watts | Illumination %, longest night, average W for a vertical panel |
| "direct-to-Earth communication windows" | Earth above terrain **and** DSN coverage | DTE % plus Gantt windows with Goldstone/Canberra/Madrid |

**Why "CLPS":** Commercial Lunar Payload Services is NASA buying rides on commercial landers.
Intuitive Machines' IM-2 *Athena* landed on Mons Mouton in March 2025 and tipped over. Astrobotic's
**Griffin-1** (carrying Astrolab's FLIP rover) targets a south-pole landing and a **late-2026
launch**, possibly during or right after the hackathon. Blue Origin's **Blue Moon MK1** will deliver
**VIPER** in early 2027. CLPS missions are short: one lunar day, limited batteries, no relay
satellite. Lighting and DTE windows decide whether they live or die, and a commercial team cannot
afford STK licenses and SPICE experts. **We are the tool for them.**

---

## 2. Why the south pole is hard (use these numbers in the pitch)

All of these come from our own validated pipeline (chapter 02):

- **The Sun never rises more than ~1.5° above the pole** (up to ~7° at sites 6° from the pole). Apollo
  landed with the Sun 8–50° up. Horizontal solar panels get **2.6 %** of full power at 1.5°, so
  landers need vertical arrays.
- **Shadows are up to ~150 km long.** A 4 km massif at a 1.5° Sun shades an area the size of a
  small country.
- **Earth bobs ±6.7° around the horizon** over 27.2 days (libration). At the pole it is below the
  horizon about half the time. **During this hackathon weekend (Nov 14–15 2026) Earth is 3.5–4.5°
  below the horizon at the pole.** A lander there could not phone home.
- **Getting the frame wrong costs 875 m** (MOON_ME vs MOON_PA), which is enough to land on the wrong
  side of a ridge.

---

## 3. Existing tools and the gap

| Tool | Strength | Why planners/educators/public struggle |
|---|---|---|
| **JPL Horizons** (web/API) | Authoritative ephemerides; supports a topocentric observer on the Moon | Text tables; you must know `coord@301`, `SITE_COORD` and quantity codes; no terrain, so "above the horizon" is wrong in craters |
| **NAIF SPICE** (toolkits) | The gold standard; used by missions | Requires programming plus correct kernels and frames |
| **Cosmographia + SPICE** | 3D, SPICE-accurate | ~0.5 GB download, catalog files to author, mission-oriented, no terrain horizon analysis |
| **JPL Moon Trek** (trek.nasa.gov/moon) | Web GIS with many LRO layers | GIS mindset (layers and tools); not built around "when is the Sun/Earth visible at this site" |
| **LROC QuickMap** | Superb imagery browsing | Imagery, not time-series lighting/comms analysis |
| **PGDA illumination products / papers** | Peer-reviewed average illumination | Static maps; GIS skills required; one time window |
| **NASA SVS visualizations** | Gorgeous south-pole lighting movies (2023–2030) | Non-interactive; fixed viewpoints and dates |
| **Ansys STK** | Full mission analysis incl. lunar comms | Commercial license, steep learning curve |
| **NASA DUST** (UE5, JSC) | Realistic south-pole sim | Distributed via software usage agreement; not a public comparison tool |
| **2023 NASA App Development Challenge** | Students visualized south-pole terrain plus comms | Earlier, student scope; no validated ephemeris or site comparison |

**The gap:** nobody combines *validated* Sun/Earth geometry, *real terrain horizons*, *side-by-side
site and date comparison* and *an interface a teacher can use in front of a class* in one free
app. That sentence is our **Impact** statement.

---

## 4. Product concept

**Working name:** **Lunar Horizon: South Pole Mission Browser** (code name `LunarHorizon`).
Alternatives: *Long Shadows*, *Earthrise Atlas*, *Terminator* (it is a real lighting term, but
people will think of the movie).

**One-liner:** *Pick any spot near the lunar south pole, any date, and see whether a lander there
gets sunlight and can talk to Earth, with proof.*

**Three modes, one app:**

| Mode | For | What changes |
|---|---|---|
| **Planner** (default) | Mission designers, CLPS teams | KPI cards, Gantt timeline, CSV export, mast-height slider, uncertainty bands |
| **Classroom** | Educators | ExplainerCards auto-open, big type, "visibility boost" lighting, guided tours |
| **Cinematic** | Public / pitch | UI fades out, camera rails, time-lapse, Quindar beeps on Earthrise |

---

## 5. Personas and jobs-to-be-done

| Persona | Job | Success = |
|---|---|---|
| **Priya, CLPS mission planner** | "Given a launch window, which of these 3 candidate spots gives the most sunlit hours *and* DTE time during our 10-day surface mission?" | Ranked comparison with longest-night and DTE % in < 1 min, exportable |
| **Mr. Okafor, high-school physics teacher** | "Explain why the south pole is special and why Earth wobbles in the sky." | A 5-minute guided tour he can run on a projector without training |
| **Sam, space fan** | "Show me what it would look like to stand there." | Astronaut-eye view, Earth over the ridge, shareable link or screenshot |
| **Judge** | "Is it accurate, usable and relevant?" | Validation badge, NASA data credits, smooth demo |

---

## 6. Feature list (prioritized)

### MVP: must exist by Sunday noon of the hackathon
1. Site picker on a 3D Moon globe with the **Artemis regions + CLPS missions + reference points**
   (chapter 03 §2).
2. Cinematic **fly-down** to the surface view of the chosen site.
3. **Physically correct Sun and Earth** in the 3D sky, driven by SPICE at the scrubbed UTC.
4. **HorizonPanorama**: 360° skyline with Sun/Earth tracks and "now" markers.
5. **Timeline** with play/pause, speed presets (1 h/s, 1 d/s, "month in 30 s") and Gantt rows for
   Sunlit / Earth visible / DSN / DTE.
6. **CompareCards** for 2–4 sites: Illumination %, Longest night, DTE %, Avg power, Next sunrise,
   Next Earthrise.
7. **Validation badge**: "Matches JPL Horizons to 0.0001°". It opens a panel with the table from
   chapter 02 §9.

### Stretch: if MVP is solid by Saturday night
8. Click **any point** → runtime horizon mask → same metrics.
9. **Mast-height slider** (2 m / 10 m / 20 m) showing how DTE and sunlight change.
10. **Astronaut-eye mode** (first person, 1/6 g) plus **dark-adapted eye** toggle with stars.
11. **Heatmap overlay** of illumination % on the terrain for the selected window.
12. ExplainerCards and a 5-step **guided tour**.
13. **Share link** and CSV export.

### Post-hackathon (mention in the pitch as the roadmap)
- Uncertainty bands from PGDA DEM error clones; eclipse flags; full-year sweeps.
- Relay satellites (lunar comms constellations) in addition to DTE.
- Rover traverse planning: "stay in sunlight along this path".
- Web build / Pixel Streaming for public outreach at scale.

---

## 7. Screen-by-screen UX spec

### 7.1 Globe & site picker (`WBP_SitePicker`)

- Full-screen Moon (Cesium Moon or SVS CGI Moon Kit) slowly rotating, south pole tilted toward the
  viewer. Real stars behind.
- Left rail: site list grouped by **Artemis regions · CLPS missions · Reference points**, each with
  a 1-line blurb and a colored kind-chip. Search box.
- Hover a site → a pin pulses on the globe and a mini-card shows lat/lon and the one-line story
  ("IM-2 Athena: landed March 2025, tipped over in a 20 m crater").
- Click → **fly-down**: about 4 s ease-in-out from orbit to 2 km above the site. Terrain detail
  sharpens and shadows lengthen as you descend. This is the first wow.

### 7.2 Surface view HUD (`WBP_Main`)

```
┌───────────────────────────────────────────────────────────────────────────────┐
│ ◀ Sites   IM-2 Athena · 84.79°S 29.20°E        2026-11-14 18:00 UTC   ⓘ  ⚙   │
│                                                                               │
│                                                                               │
│                 [ 3D photoreal terrain · Sun · Earth · horizon ring ]         │
│                                                                               │
│                                                       ┌─ Now ─────────────┐   │
│                                                       │ ☀ 1.21° ▲ 72% lit │   │
│                                                       │ 🜨 −0.8° ✕ no DTE  │   │
│                                                       │ Earthrise in 9h12m│   │
│                                                       └───────────────────┘   │
│ ┌─ Horizon panorama ───────────────────────────────────────────────────────┐  │
│ │   N        E         S         W        N   ☀━━━━━   🜨┄┄┄┄  ▓ terrain    │  │
│ │ ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓  │  │
│ └──────────────────────────────────────────────────────────────────────────┘  │
│ ┌─ Timeline ─  ▶ ⏸  1h/s · 1d/s · Month/30s ───────────────────────────────┐  │
│ │ Sunlit  ████████░░░███████████████░░░░████████████                        │  │
│ │ Earth   ░░░░░████████████░░░░░░░░░░░░░████████████                        │  │
│ │ DSN     ██████████████████████████████████████████                        │  │
│ │ DTE     ░░░░░████████████░░░░░░░░░░░░░████████████       ▲ now            │  │
│ └──────────────────────────────────────────────────────────────────────────┘  │
└───────────────────────────────────────────────────────────────────────────────┘
```

- The **Now** card mirrors the 3D scene in numbers. Every number has a tooltip explaining it in
  plain words.
- Clicking the panorama at an azimuth rotates the camera to look that way, which links chart and
  world.
- Dragging the timeline scrubs time. Shadows sweep live. That is the second wow.

### 7.3 HorizonPanorama (`WBP_HorizonPanorama`)

- X = azimuth 0–360° (N at both ends, E, S, W labeled). Y = elevation −10° to +15° (exaggerated
  vertical scale, labeled on the axis).
- Dark terrain silhouette from the horizon mask, a thin line at 0°, the **Sun track** (amber, solid
  where visible, dotted where blocked) and the **Earth track** (blue), with day ticks.
- Current Sun (●, true 0.53° size) and Earth (◐ with its real phase).
- Toggle "mast 10 m": the silhouette drops and the tracks emerge. People see in one second why
  masts matter.

### 7.4 Timeline (`WBP_Timeline`)

- Window presets: *This lunar day* · *Mission window (custom)* · *Next 30 days* · *Full year*
  (stretch).
- Four Gantt rows. Hover a block → "Sunlit 3 d 4 h, 14 Nov 02:00 → 17 Nov 06:00 UTC".
- Hackathon date marker plus "today" marker.

### 7.5 CompareCards (`WBP_CompareCards`)

- 2–4 columns. Rows: Illumination %, Longest night (h), DTE %, DSN %, Avg power (W/m² panel),
  Next sunrise, Next Earthrise. The **best value in each row is highlighted**.
- Each card has a sparkline of SunVisibleFraction over the window.
- "Same date window for all" is always on and shown, so comparisons stay fair.
- Export CSV button.

### 7.6 ExplainerCard (`WBP_ExplainerCard`)

Short, friendly, accurate. Auto-open in Classroom mode. Six cards to write first:

1. **Why is the Sun always on the horizon?** "The Moon's axis is tilted only 1.5°. At the pole, the
   Sun circles the horizon once a month like a lighthouse beam that never rises."
2. **Why does Earth bob up and down?** "The Moon wobbles slightly as it orbits (libration). From the
   pole, Earth rises and sets by about ±7° over 27 days."
3. **Why vertical solar panels?** "Light arriving almost sideways barely hits a flat panel: 2.6 % at a
   1.5° Sun. Stand the panel up and you get almost 100 %."
4. **What's a horizon mask?** "The jagged line of mountains around you. If the Sun or Earth is below
   it, you can't see them, even if they are above the 'flat' horizon."
5. **What's DTE?** "Direct-to-Earth: talking to a big antenna on Earth with no relay satellite.
   Earth must be above your local horizon and the antenna must see the Moon."
6. **Why does it look so close?** "There is no air, so there is no haze. A mountain 50 km away looks
   as sharp as one 500 m away."

### 7.7 Astronaut-eye mode & time-lapse (stretch)

- WASD/gamepad, 1/6 g, a slow bouncing gait, suit audio, helmet-visor vignette (subtle).
- Time-lapse: "Lunar day in 20 seconds". Shadows rotate 360° around you. Earth bobs. A Quindar beep
  sounds at Earthrise.

---

## 8. Visual design language

| Element | Spec |
|---|---|
| Theme | Near-black UI (#0B0C0E panels at 85 % opacity, 1 px #2A2D33 borders). The Moon is the hero; UI never competes |
| **Sun** | Amber #FFA81A, **filled circle** icon, solid lines |
| **Earth** | Blue #3F9BFF, **ring/phase** icon, dashed lines |
| DSN | Neutral #C8CCD2, antenna icon |
| DTE | Blue #3F9BFF with a white outline (Earth ∧ DSN) |
| Shadow/blocked | Dotted variant of the same color |
| Color-blind safety | Amber vs blue is distinguishable in all common CVD types, and **shape + line style** also encode the meaning, never color alone |
| Typography | Inter (UI) + JetBrains Mono or IBM Plex Mono with **tabular figures** for numbers (no jitter while scrubbing) |
| Motion | 150–250 ms ease-out for UI. Camera moves 2–4 s ease-in-out. Nothing bounces |
| Units | Degrees with 2 decimals, UTC always labeled, hours as "3 d 4 h" |
| Accessibility | Full keyboard nav (CommonUI), 16 px minimum text (24 px in Classroom mode), subtitles on all audio/video, reduced-motion toggle (instant cuts instead of fly-downs) |

---

## 9. Presets

Ship with:
- **Artemis candidate regions (9)**. Artemis IV, target 2028, is now the first crewed landing.
- **CLPS & international**: IM-2 Athena (actual + intended), IM-1 Odysseus, Griffin-1 (when its site
  is published), Blue Moon MK1/VIPER (when published), Chandrayaan-3, Chang'e-7 target.
- **Reference points** with published illumination (Bussey 2010: Shackleton ridge A–D, Malapert M1)
  plus the geographic pole. These double as the validation set.
- **Saved comparisons**: "Artemis IV shortlist", "Peaks of near-eternal light", "Hackathon weekend".

---

## 10. How each feature scores (judging rubric map)

The rubric (Judging & Awards Guide) scores **1–5** on each criterion:

| Criterion | Rubric essence | What earns the 5 |
|---|---|---|
| **Impact** | Size of the problem; how many it helps or inspires | Planners **and** classrooms **and** the public; CLPS-relevant now (Griffin-1, VIPER); free and open |
| **Creativity** | Novel concept or meaningful improvement | Validated science + photoreal game engine; horizon-relative panorama; mast slider; Quindar Earthrise cue |
| **Validity** | Grounded in science; feasible; works | Matches JPL Horizons to 0.0001°; LOLA DEMs with stated errors; reproducible `lunar_sky.py`; render ↔ math agreement check |
| **Relevance** | Fully addresses the challenge; NASA data is core | Directly answers every verb of the statement (§1); LRO/LOLA/LROC/SPICE/DSN data at the core, not as decoration |
| **Presentation** | Clear, engaging storytelling | 30-s video with a story arc; Classroom mode; clean project page with credits |

The guide also says to pay equal attention to Presentation. Assign one person to the video from
hour 1.

---

## 11. Pitch & demo

### 11.1 The 30-second video (must include English subtitles; host on YouTube, link must be public)

This follows the Project Submission Guide's beats: intro (0:00) → challenge (0:02) → project name
(0:05) → how it works with NASA data (0:10) → the future (0:20–0:30).

| t (s) | Beat | Shot | On-screen text / subtitle |
|---|---|---|---|
| 0–2 | Intro | A sliver of white light creeps over a jagged ridge. Lower-third: team name and members | "We're Team ___." |
| 2–5 | Challenge | Wide shot: the Sun skims the horizon, shadows kilometers long; Earth sinks behind a massif | "At the lunar south pole, the Sun never rises and Earth slips out of sight." |
| 5–10 | Name + tagline | Globe spins, click "IM-2 Athena", fly-down; title card fades over the landing | "Lunar Horizon: pick a site, pick a date, see if you'll have power and a line home." |
| 10–20 | How it works + NASA data | Timeline scrubbed, shadows sweep, Gantt bars fill; CompareCards highlight the best site; 2 s on the validation table next to JPL Horizons; credits strip "NASA LRO LOLA · LROC · JPL SPICE · DSN" | "Real NASA LRO terrain and JPL ephemerides, verified to 0.0001°, compared side by side in seconds." |
| 20–30 | Future | Astronaut-eye view: Earthrise over the ridge, Quindar beep; quick cuts: classroom, CLPS lander, roadmap icons | "For CLPS teams, Artemis planners and every classroom. Plan the next landing." |

Record it with Movie Render Graph (chapter 05 §10). Burn in subtitles. Export 1080p60 H.264.
Keep it **≤ 30.0 s**. Longer is disqualified from Global Judging.

### 11.2 Seven-slide alternative (title slide counts)

1. Title, team, challenge name.
2. The problem: 1.5° Sun, Earth bobbing, 150 km shadows (one striking render).
3. Who suffers today: tools table, condensed to 3 icons.
4. Our app: screenshot plus the 3 modes.
5. How it works: architecture diagram (NASA data → SPICE → UE5).
6. Proof: the Horizons validation table plus a "real LROC vs our render" side by side.
7. Impact and roadmap: CLPS/Artemis use, classrooms, open source; QR code to the video and repo.

### 11.3 Live demo script (2 minutes, for local judging)

1. (10 s) Globe spinning. "This is the real Moon, from NASA's LRO."
2. (15 s) Click IM-2 Athena → fly-down. "Here's where Intuitive Machines landed last year."
3. (25 s) Scrub the timeline across this weekend. "Watch the Sun skim the ridge … and Earth is
   *below* the horizon right now. A lander here couldn't call home this weekend."
4. (20 s) Mast slider 2 → 10 m. Panorama: Earth emerges. "Ten-meter mast? Now you can."
5. (25 s) Compare IM-2 vs Malapert vs Shackleton ridge. "Malapert keeps Earth in view longer; the
   ridge gets more sun. Pick based on your mission."
6. (15 s) Validation badge → table. "Our numbers match JPL Horizons to a ten-thousandth of a
   degree."
7. (10 s) Astronaut mode, Earthrise, beep. "And anyone can stand there."

### 11.4 Killer facts list (memorize)

- Sun ≤ 1.5° at the pole. Flat panels get 2.6 %. Shadows up to 150 km.
- Earth ±6.7°, below the horizon ~half the time at the pole, and below it **this weekend**.
- MOON_ME vs MOON_PA = 875 m.
- 0.0001° agreement with JPL Horizons.
- LOLA 5 m DEMs: 0.3–0.5 m median height error.

---

## 12. What *not* to build (scope protection)

- No accounts, logins or cloud saves. Share links are enough.
- No custom physics for lander touchdown.
- No full Hapke shading model in-engine (approximation in chapter 05).
- No mobile build. Pixel Streaming covers phones.
- No relay-satellite network in the MVP (roadmap slide only).
- Don't let the tech artist chase 8K textures before the lighting is physically right.

## Sources

- Challenge listing (2026): <https://www.spaceappschallenge.org/2026/challenges/clps-lunar-mission-browser/>
- Judging & Awards Guide (criteria, 1–5 scale): <https://www.spaceappschallenge.org/resources/judging-awards-guide/>
- Project Submission Guide (30-s video / 7 slides, English subtitles, public links): Space Apps resources
- CLPS / Griffin-1 / Blue Moon MK1 + VIPER status (Sep–Oct 2026): NASA CLPS updates; Wikipedia "Commercial Lunar Payload Services"
- IM-2 landing site: <https://lroc.im-ldi.com/images/1408>
- NASA SVS south-pole lighting visualizations: <https://svs.gsfc.nasa.gov/>
- DUST: <https://software.nasa.gov/software/MSC-27522-1>
