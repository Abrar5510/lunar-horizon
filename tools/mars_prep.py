"""Fetch NASA MOLA MEGDR topography for MarsMap and bake the globe textures.

    python tools/mars_prep.py

Writes MarsMap/Content/Data/megt90n000{cb,eb}.img (4 and 16 px/deg, read at runtime) and
dem/mars/T_MarsMola.png, T_MarsNatural.png (imported into Unreal by tools/mars_ue_setup.py).
"""
import pathlib
import urllib.request

import numpy as np
from PIL import Image

ROOT = pathlib.Path(__file__).resolve().parent.parent
DATA = ROOT / "MarsMap/Content/Data"
OUT = ROOT / "dem/mars"
PDS = "https://pds-geosciences.wustl.edu/mgs/mgs-m-mola-5-megdr-l3-v1/mgsl_300x/"
FILES = {"meg004/megt90n000cb": 1440, "meg016/megt90n000eb": 5760}
RADIUS_M = 3396000.0

# (elevation m, RGB): MOLA-style map colours and a rust "natural" look
MOLA = [(-8000, (40, 30, 120)), (-5500, (40, 90, 210)), (-3500, (60, 190, 215)), (-1500, (80, 190, 90)),
        (0, (225, 220, 90)), (2000, (230, 160, 60)), (4500, (200, 85, 45)), (8000, (140, 75, 60)),
        (13000, (185, 165, 155)), (21500, (255, 255, 255))]
NATURAL = [(-8000, (95, 62, 45)), (-3000, (135, 80, 52)), (0, (178, 104, 60)), (3000, (192, 124, 78)),
           (8000, (196, 150, 108)), (21500, (215, 190, 160))]


def ramp(h, stops):
    xs = [s[0] for s in stops]
    return np.stack([np.interp(h, xs, [s[1][c] for s in stops]) for c in range(3)], -1)


def hillshade(h):
    ppd = h.shape[1] / 360
    dy = 2 * np.pi * RADIUS_M / 360 / ppd
    lat = 90 - (np.arange(h.shape[0]) + 0.5) / ppd
    dx = np.maximum(dy * np.cos(np.radians(lat)), dy * 0.05)[:, None]
    ex = 4.0  # vertical exaggeration for the relief shading only
    dzde = (np.roll(h, -1, 1) - np.roll(h, 1, 1)) / (2 * dx) * ex
    north, south = np.vstack([h[:1], h[:-1]]), np.vstack([h[1:], h[-1:]])
    dzdn = (north - south) / (2 * dy) * ex
    light = np.array([-0.574, 0.574, 0.574])  # from the north-west, 35 deg up
    return np.clip((-dzde * light[0] - dzdn * light[1] + light[2]) / np.sqrt(dzde**2 + dzdn**2 + 1), 0, 1)


def main():
    DATA.mkdir(parents=True, exist_ok=True)
    OUT.mkdir(parents=True, exist_ok=True)
    for rel, width in FILES.items():
        dst = DATA / (rel.split("/")[1] + ".img")
        if (dst.stat().st_size if dst.exists() else 0) != width * width:  # int16, height = width/2
            print("downloading", PDS + rel + ".img")
            urllib.request.urlretrieve(PDS + rel + ".img", dst)
        assert dst.stat().st_size == width * width, f"{dst} has the wrong size"

    h = np.fromfile(DATA / "megt90n000eb.img", ">i2").reshape(2880, 5760).astype(np.float32)
    shade = hillshade(h)[..., None]
    lat = (90 - (np.arange(h.shape[0]) + 0.5) / 16)[:, None, None]
    ice = np.clip((lat - 80) / 5, 0, 1) + np.clip((-lat - 83) / 4, 0, 1)   # rough residual polar caps

    for name, stops, lo, gain in (("T_MarsMola", MOLA, 0.35, 0.9), ("T_MarsNatural", NATURAL, 0.55, 0.6)):
        rgb = ramp(h, stops) * (lo + gain * shade)
        if name == "T_MarsNatural":
            rgb = rgb * (1 - ice) + np.array([238, 232, 225]) * (0.75 + 0.25 * shade) * ice
        img = Image.fromarray(np.clip(rgb, 0, 255).astype(np.uint8)).resize((4096, 2048), Image.LANCZOS)
        img.save(OUT / f"{name}.png")
        print("wrote", OUT / f"{name}.png")


if __name__ == "__main__":
    main()
