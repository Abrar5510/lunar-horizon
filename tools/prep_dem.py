"""Sites.json + LOLA DEMs -> per-site terrain grids, Unreal hero heightmap, true site height.

  python tools/prep_dem.py LunarHorizon/Content/Data/Sites.json dem sites

Every DEM in dem_dir that overlaps a site is composited into a grid centred on it, coarse first
and finer on top, so each site gets the best LOLA data there is: PGDA 5 m site DEMs, the 87.5S 5 m
mosaic, then the 20/60 m polar GDRs. Per site it writes to out_dir:
  <Id>_near5m.npy   4801 x 4801 float32, 5 m, +-12 km   (near-field horizon, hero tile)
  <Id>_far60m.npy   5001 x 5001 float32, 60 m, +-150 km (far-field horizon)
  <Id>_hero.png     4033 x 4033 16-bit Landscape heightmap (+-10 km) and <Id>_hero.json placement
and writes the DEM height at the site into Sites.json "HeightM".
Grids are row 0 = +y (grid north edge), col 0 = -x, centre pixel = the site.
"""
import glob, json, math, os, sys, warnings
import numpy as np, rasterio
from rasterio.enums import Resampling
from rasterio.errors import NotGeoreferencedWarning
from rasterio.transform import from_origin
from rasterio.warp import reproject
from rasterio.windows import Window, WindowError, from_bounds

R_M = 1737400.0
CRS = "+proj=stere +lat_0=-90 +lat_ts=-90 +lon_0=0 +k=1 +x_0=0 +y_0=0 +R=1737400 +units=m +no_defs"
NEAR = (4801, 5.0)    # (size px, metres/px)
FAR = (5001, 60.0)
HERO = 4033           # Unreal-friendly Landscape size, 20.165 km at 5 m


def stereo_xy(lat, lon):
    rho = 2 * R_M * math.tan(math.pi / 4 + math.radians(lat) / 2)
    return rho * math.sin(math.radians(lon)), rho * math.cos(math.radians(lon))


def sources(dem_dir):
    """[(path, metres per DN, res_m)] coarse first. PDS GDR float grids are in km."""
    out = []
    for p in glob.glob(os.path.join(dem_dir, "*.tif")) + glob.glob(os.path.join(dem_dir, "*_float.lbl")):
        with rasterio.open(p) as ds:
            out.append((p, 1000.0 if p.endswith(".lbl") else 1.0, ds.res[0]))
    return sorted(out, key=lambda s: -s[2])


def grid(srcs, x, y, n, px):
    """n x n grid centred on (x, y); returns (heights_m, names of contributing DEMs)."""
    half = n * px / 2
    dst_t = from_origin(x - half, y + half, px, px)
    out = np.full((n, n), np.nan, np.float32)
    used = []
    for path, scale, res in srcs:
        with rasterio.open(path) as ds:
            try:
                win = from_bounds(x - half - 4 * res, y - half - 4 * res, x + half + 4 * res,
                                  y + half + 4 * res, ds.transform)
                win = win.round_offsets().round_lengths().intersection(Window(0, 0, ds.width, ds.height))
            except WindowError:
                continue
            f = max(1, int(px // res))            # decimate sources much finer than the target
            shape = (max(1, int(win.height) // f), max(1, int(win.width) // f))
            a = ds.read(1, window=win, out_shape=shape, resampling=Resampling.average, masked=True)
            a = a.astype(np.float32).filled(np.nan) * np.float32(scale)
            t = ds.window_transform(win) * rasterio.Affine.scale(win.width / shape[1], win.height / shape[0])
        tmp = np.full_like(out, np.nan)
        reproject(a, tmp, src_transform=t, src_crs=CRS, dst_transform=dst_t, dst_crs=CRS,
                  src_nodata=np.nan, dst_nodata=np.nan, resampling=Resampling.bilinear)
        if np.isfinite(tmp).any():
            out = np.where(np.isnan(tmp), out, tmp)
            used.append(os.path.basename(path))
    return out, used


def write_hero(near, site, used, out_dir):
    o = (near.shape[0] - HERO) // 2
    hero = near[o:o + HERO, o:o + HERO]
    if np.isnan(hero).any():
        print(f"{site['Id']:16s} no hero tile: DEM coverage has gaps within 10 km")
        return
    hmin, hmax = float(hero.min()), float(hero.max())
    png = np.round((hero - hmin) / max(hmax - hmin, 1e-3) * 65535).astype(np.uint16)
    warnings.simplefilter("ignore", NotGeoreferencedWarning)   # a plain PNG for Unreal, no geotransform wanted
    with rasterio.open(os.path.join(out_dir, site["Id"] + "_hero.png"), "w", driver="PNG",
                       width=HERO, height=HERO, count=1, dtype="uint16") as ds:
        ds.write(png, 1)
    # Landscape origin is its corner. Put the site pixel at the georeference origin (East-South-Up)
    # and yaw by -lon so grid north lines up with local north (docs/04 section 10.1).
    lam, c = math.radians(site["LonDeg"]), (HERO - 1) / 2 * 500.0
    json.dump(dict(SizePx=HERO, PixelM=5.0, HminM=hmin, HmaxM=hmax,
                   ScaleXYcm=500.0, ScaleZpct=(hmax - hmin) / 512 * 100,
                   ActorLocationCm=[-c * (math.cos(lam) + math.sin(lam)), -c * (math.cos(lam) - math.sin(lam)),
                                    ((hmin + hmax) / 2 - site["HeightM"]) * 100],
                   ActorYawDeg=-site["LonDeg"], Sources=used),
              open(os.path.join(out_dir, site["Id"] + "_hero.json"), "w"), indent=2)


def main(sites_json, dem_dir, out_dir):
    os.makedirs(out_dir, exist_ok=True)
    srcs = sources(dem_dir)
    sites = json.load(open(sites_json))
    for s in sites:
        x, y = stereo_xy(s["LatDeg"], s["LonDeg"])
        near, used = grid(srcs, x, y, *NEAR)
        far, _ = grid(srcs, x, y, *FAR)
        h = near[NEAR[0] // 2, NEAR[0] // 2]
        if not np.isfinite(h):
            print(f"{s['Id']:16s} SKIPPED: no DEM in {dem_dir} covers it")
            continue
        s["HeightM"] = round(float(h), 1)
        np.save(os.path.join(out_dir, s["Id"] + "_near5m.npy"), near)
        np.save(os.path.join(out_dir, s["Id"] + "_far60m.npy"), far)
        write_hero(near, s, used, out_dir)
        print(f"{s['Id']:16s} h={s['HeightM']:8.1f} m  finest={used[-1]}")
    with open(sites_json, "w") as f:
        f.write("[\n" + ",\n".join("  " + json.dumps(s) for s in sites) + "\n]\n")


def selftest():
    x, y = stereo_xy(-84.7906, 29.1957)                 # IM-2, docs/03 section 4.2
    assert abs(x - 77108) < 2 and abs(y - 137993) < 2, (x, y)
    assert stereo_xy(-90, 0) == (0.0, 0.0)
    print("selftest ok")


if __name__ == "__main__":
    selftest() if sys.argv[1:] == ["--selftest"] else main(*sys.argv[1:4])
