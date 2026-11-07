"""Fetch NASA data for MarsMap and bake everything the Unreal project loads.

    python tools/mars_prep.py

Downloads (cached in dem/mars and MarsMap/Content/Data):
  MOLA MEGDR topography 4/16/64 px/deg (PDS), Viking MDIM colour mosaic 925 m (USGS),
  Phobos shape model phobos_3_3.bds (NAIF DSK, Ernst et al. 2023).
Writes:
  MarsMap/Content/Data/megt90n000{cb,eb}.img   elevation grids read at runtime
  MarsMap/Content/Data/{phobos,deimos}.bin     moon meshes (int32 nv, nt; float32 xyz km; int32 tris)
  dem/mars/T_MarsViking.png (16k, real colour), T_MarsNormal.png (16k, MOLA relief), T_MarsMola.png (4k map colours)
Then run tools/mars_ue_setup.py to import the textures into Unreal.
"""
import math
import pathlib
import struct
import urllib.request

import numpy as np
from PIL import Image

Image.MAX_IMAGE_PIXELS = None
ROOT = pathlib.Path(__file__).resolve().parent.parent
DATA = ROOT / "MarsMap/Content/Data"
OUT = ROOT / "dem/mars"
KERNELS = ROOT / "kernels"
PDS = "https://pds-geosciences.wustl.edu/mgs/mgs-m-mola-5-megdr-l3-v1/mgsl_300x/"
VIKING = "https://asc-pds-services.s3.us-west-2.amazonaws.com/mosaic/Mars_Viking_ClrMosaic_global_925m.tif"
PHOBOS = "https://naif.jpl.nasa.gov/pub/naif/generic_kernels/dsk/satellites/phobos_3_3.bds"
RADIUS_M = 3396000.0
TEX = (16384, 8192)

MOLA_COLOURS = [(-8000, (40, 30, 120)), (-5500, (40, 90, 210)), (-3500, (60, 190, 215)), (-1500, (80, 190, 90)),
                (0, (225, 220, 90)), (2000, (230, 160, 60)), (4500, (200, 85, 45)), (8000, (140, 75, 60)),
                (13000, (185, 165, 155)), (21500, (255, 255, 255))]


def fetch(url, dst, size=None):
    if dst.exists() and (size is None or dst.stat().st_size == size):
        return dst
    print("downloading", url)
    urllib.request.urlretrieve(url, dst)
    assert size is None or dst.stat().st_size == size, f"{dst} has the wrong size"
    return dst


def gradients(h):
    """East and north slope (dz/dx) of an equirectangular grid, metres per metre."""
    ppd = h.shape[1] / 360
    dy = 2 * np.pi * RADIUS_M / 360 / ppd
    lat = 90 - (np.arange(h.shape[0]) + 0.5) / ppd
    dx = np.maximum(dy * np.cos(np.radians(lat)), dy * 0.05)[:, None].astype(np.float32)
    dzde = (np.roll(h, -1, 1) - np.roll(h, 1, 1)) / (2 * dx)
    dzdn = (np.vstack([h[:1], h[:-1]]) - np.vstack([h[1:], h[-1:]])) / (2 * dy)
    return dzde, dzdn


def resize(a, size):
    return np.asarray(Image.fromarray(a).resize(size, Image.BOX))


def bake_mola_map():
    h = np.fromfile(DATA / "megt90n000eb.img", ">i2").reshape(2880, 5760).astype(np.float32)
    e, n = gradients(h * 4)
    shade = np.clip((-e * -0.574 - n * 0.574 + 0.574) / np.sqrt(e * e + n * n + 1), 0, 1)[..., None]
    xs = [s[0] for s in MOLA_COLOURS]
    rgb = np.stack([np.interp(h, xs, [s[1][c] for s in MOLA_COLOURS]) for c in range(3)], -1) * (0.35 + 0.9 * shade)
    Image.fromarray(np.clip(rgb, 0, 255).astype(np.uint8)).resize((4096, 2048), Image.LANCZOS).save(OUT / "T_MarsMola.png")


def bake_normal():
    """Tangent-space normal map (Unreal/DirectX: +X east = +U, +Y south = +V) from MOLA 64 px/deg."""
    tiles = {}
    for t in ("megt90n000gb", "megt90n180gb", "megt00n000gb", "megt00n180gb"):
        fetch(PDS + "meg064/" + t + ".img", OUT / (t + ".img"), 11520 * 5760 * 2)
        tiles[t] = np.fromfile(OUT / (t + ".img"), ">i2").reshape(5760, 11520)
    h = np.block([[tiles["megt90n000gb"], tiles["megt90n180gb"]], [tiles["megt00n000gb"], tiles["megt00n180gb"]]])
    h = resize(h.astype(np.float32), TEX)
    e, n = gradients(h)
    strength = 2.0   # relief is subtle at 1.3 km/px; double it so craters read under low sun
    nx, ny, nz = -e * strength, n * strength, np.ones_like(e)
    inv = 1 / np.sqrt(nx * nx + ny * ny + nz * nz)
    rgb = np.stack([nx * inv, ny * inv, nz * inv], -1) * 127.5 + 127.5
    Image.fromarray(rgb.astype(np.uint8)).save(OUT / "T_MarsNormal.png", compress_level=1)


def bake_colour():
    import rasterio
    with rasterio.open(fetch(VIKING, OUT / VIKING.rsplit("/", 1)[1], 797888177)) as d:
        img = np.moveaxis(d.read(), 0, -1)
        starts_at_180w = d.bounds.left < 0   # projected metres or degrees, either way negative = -180
    if starts_at_180w:                        # our UV u=0 is 0 deg E
        img = np.roll(img, img.shape[1] // 2, axis=1)
    rgb = resize(img, TEX).astype(np.float32) / 255
    # Grade toward true colour: the Viking colourisation paints dark albedo regions navy and the caps pink.
    # Re-map brightness onto tones sampled from orbital true-colour images (dark basalt -> butterscotch dust),
    # keep 30% of the original colour for regional variety, and whiten bright high-latitude ice.
    lum = rgb @ np.array([0.30, 0.55, 0.15], np.float32)
    lo, hi = np.percentile(lum, [1, 99])
    t = np.clip((lum - lo) / (hi - lo), 0, 1)[..., None]
    ramp = np.array([[52, 40, 34], [118, 78, 55], [176, 116, 74], [214, 160, 112]], np.float32) / 255
    graded = np.stack([np.interp(t[..., 0], [0, 0.35, 0.7, 1], ramp[:, c]) for c in range(3)], -1)
    out = graded * 0.7 + rgb * 0.3 * (graded.mean(-1, keepdims=True) / np.maximum(rgb.mean(-1, keepdims=True), 1e-3))
    lat = np.abs(90 - (np.arange(TEX[1]) + 0.5) * 180 / TEX[1])[:, None, None]
    ice = np.clip((t - 0.72) / 0.15, 0, 1) * np.clip((lat - 60) / 10, 0, 1)
    out = out * (1 - ice) + np.array([0.93, 0.91, 0.88], np.float32) * (0.75 + 0.25 * t) * ice
    Image.fromarray(np.clip(out * 255, 0, 255).astype(np.uint8)).save(OUT / "T_MarsViking.png", compress_level=1)


def write_mesh(path, v, t):
    with open(path, "wb") as f:
        f.write(struct.pack("<ii", len(v), len(t)))
        f.write(np.asarray(v, "<f4").tobytes())
        f.write(np.asarray(t, "<i4").tobytes())


def moons():
    import spiceypy as sp
    fetch(PHOBOS, KERNELS / "phobos_3_3.bds")
    h = sp.dasopr(str(KERNELS / "phobos_3_3.bds"))
    dla = sp.dlabfs(h)
    nv, nt = sp.dskz02(h, dla)
    v = np.array(sp.dskv02(h, dla, 1, nv), dtype=np.float64)          # km, IAU_PHOBOS (+X ~ toward Mars)
    t = np.array(sp.dskp02(h, dla, 1, nt), dtype=np.int64) - 1          # 1-based plates
    write_mesh(DATA / "phobos.bin", v, t)
    sp.dascls(h)
    print(f"phobos: {nv} vertices, {nt} plates")

    # Deimos: no public plate model on NAIF yet, so an approximation: Ernst et al. (2023) best-fit
    # ellipsoid 8.04 x 5.89 x 5.11 km with smooth low-frequency relief (Deimos is regolith-blanketed).
    rng = np.random.default_rng(7)
    lat, lon = np.meshgrid(np.linspace(-90, 90, 181), np.linspace(0, 360, 361), indexing="ij")
    u = np.stack([np.cos(np.radians(lat)) * np.cos(np.radians(lon)), np.cos(np.radians(lat)) * np.sin(np.radians(lon)), np.sin(np.radians(lat))], -1)
    bump = np.zeros(lat.shape)
    for _ in range(60):                                                  # random smooth bumps and shallow craters
        c = rng.normal(size=3); c /= np.linalg.norm(c)
        w = rng.uniform(0.15, 0.6)
        d = np.arccos(np.clip(u @ c, -1, 1))
        bump += rng.uniform(-0.12, 0.08) * np.exp(-(d / w) ** 2)
    v = u * np.array([8.04, 5.89, 5.11]) * (1 + bump)[..., None]
    W = lon.shape[1]
    idx = lambda j, i: j * W + i
    tris = [(idx(j, i), idx(j + 1, i), idx(j, i + 1)) for j in range(180) for i in range(360)] + \
           [(idx(j, i + 1), idx(j + 1, i), idx(j + 1, i + 1)) for j in range(180) for i in range(360)]
    write_mesh(DATA / "deimos.bin", v.reshape(-1, 3), tris)

    # Circular-orbit fits in the Mars body-fixed frame (pasted into MarsCore.h MoonOrbits)
    for k in ("naif0012.tls", "pck00011.tpc", "de440s.bsp", "mar099s.bsp"):
        sp.furnsh(str(KERNELS / k))
    t0 = sp.str2et("2026-11-14T00:00:00")
    for moon in ("PHOBOS", "DEIMOS"):
        ts = t0 + np.arange(0, 2 * 365 * 86400, 3600.0)
        p = np.array([sp.spkpos(moon, x, "IAU_MARS", "NONE", "MARS")[0] for x in ts])
        lon = np.unwrap(np.arctan2(p[:, 1], p[:, 0]))
        A = np.vstack([np.ones_like(ts), (ts - t0) / 86400]).T
        c, *_ = np.linalg.lstsq(A, lon, rcond=None)
        err = np.degrees(np.abs(lon - A @ c)).max()
        print(f"{moon}: r {np.linalg.norm(p, axis=1).mean():.1f} km, lon0 {math.degrees(c[0]) % 360:.4f} deg, "
              f"rate {math.degrees(c[1]):.7f} deg/day, worst error {err:.2f} deg over 2 years")


def main():
    DATA.mkdir(parents=True, exist_ok=True)
    OUT.mkdir(parents=True, exist_ok=True)
    for rel, width in {"meg004/megt90n000cb": 1440, "meg016/megt90n000eb": 5760}.items():
        fetch(PDS + rel + ".img", DATA / (rel.split("/")[1] + ".img"), width * width)
    for step in (bake_mola_map, bake_normal, bake_colour, moons):
        print("==", step.__name__)
        step()


if __name__ == "__main__":
    main()
