"""Reference science pipeline for LunarHorizon (Sun/Earth sky, horizon masks, power & DTE metrics).

This is the ground truth the Unreal C++ port (LHScience module) must match.

  python lunar_sky.py --kernels ./kernels --validate          # cross-check vs JPL Horizons
  python lunar_sky.py --kernels ./kernels --site -89.45 222.69 --start 2026-11-01 --days 30
  python lunar_sky.py --selftest                              # no kernels needed

Kernels (https://naif.jpl.nasa.gov/pub/naif/generic_kernels/):
  lsk/naif0012.tls  pck/pck00011.tpc  pck/moon_pa_de440_200625.bpc  fk/satellites/moon_de440_250416.tf
  spk/planets/de440s.bsp  spk/stations/earthstns_itrf93_260814.bsp  fk/stations/earth_topo_260814.tf
  pck/earth_latest_high_prec.bpc
"""
import argparse, csv, json, math, os, urllib.parse, urllib.request
import numpy as np

R_MOON_KM = 1737.4
SOLAR_CONST = 1361.0          # W/m^2 at 1 AU (IAU 2015 B3 nominal)
AU_KM = 149597870.7
SUN_RADIUS_KM = 695700.0
DSN = ("DSS-14", "DSS-43", "DSS-63")   # Goldstone, Canberra, Madrid 70 m
DSN_MIN_EL_DEG = 10.0

KERNELS = ["naif0012.tls", "pck00011.tpc", "moon_pa_de440_200625.bpc", "moon_de440_250416.tf",
           "de440s.bsp", "earthstns_itrf93_260814.bsp", "earth_topo_260814.tf", "earth_latest_high_prec.bpc"]


def load_kernels(kdir):
    import spiceypy as sp
    for k in KERNELS:
        sp.furnsh(os.path.join(kdir, k))
    return sp


def site_vector_km(lat_deg, lon_deg, height_m=0.0):
    """Body-fixed MOON_ME position of a surface site (sphere, R = 1737.4 km)."""
    r = R_MOON_KM + height_m / 1000.0
    la, lo = math.radians(lat_deg), math.radians(lon_deg)
    return [r * math.cos(la) * math.cos(lo), r * math.cos(la) * math.sin(lo), r * math.sin(la)]


def az_el(sp, target, et, obspos, obsctr="MOON", obsref="MOON_ME"):
    """Apparent azimuth (from north, clockwise) / elevation in degrees, range in km."""
    azl, _lt = sp.azlcpo("ELLIPSOID", target, et, "LT+S", False, True, obspos, obsctr, obsref)
    return math.degrees(azl[1]), math.degrees(azl[2]), azl[0]


def disk_visible_fraction(center_el_deg, horizon_el_deg, radius_deg):
    """Fraction of a uniform disk above a (locally flat) horizon line. 0..1."""
    h = (center_el_deg - horizon_el_deg) / radius_deg      # center height in disk radii
    if h >= 1.0:
        return 1.0
    if h <= -1.0:
        return 0.0
    # area of circular segment below the line, normalised by pi
    below = (math.acos(h) - h * math.sqrt(1.0 - h * h)) / math.pi
    return 1.0 - below


def load_mask(path):
    """Content/Data/Horizons/<SiteId>.csv -> (step_deg, elevations[np.ndarray])."""
    rows = list(csv.DictReader(open(path)))
    el = np.array([float(r["elevation_deg"]) for r in rows])
    return 360.0 / len(el), el


def mask_at(mask, az_deg):
    if mask is None:
        return 0.0
    step, el = mask
    x = (az_deg % 360.0) / step
    i = int(x) % len(el)
    f = x - int(x)
    return el[i] * (1 - f) + el[(i + 1) % len(el)] * f


def sky_state(sp, et, obspos, mask=None):
    s_az, s_el, s_rng = az_el(sp, "SUN", et, obspos)
    e_az, e_el, e_rng = az_el(sp, "EARTH", et, obspos)
    sun_r = math.degrees(math.asin(SUN_RADIUS_KM / s_rng))
    sun_frac = disk_visible_fraction(s_el, mask_at(mask, s_az), sun_r)
    earth_vis = e_el > mask_at(mask, e_az)                  # Earth centre above terrain
    return dict(sun_az=s_az, sun_el=s_el, sun_km=s_rng, sun_frac=sun_frac,
                earth_az=e_az, earth_el=e_el, earth_km=e_rng, earth_vis=earth_vis)


def dsn_sees_moon(sp, et):
    """True if any 70 m DSN antenna has the Moon above DSN_MIN_EL_DEG."""
    for st in DSN:
        pos, _ = sp.spkpos(st, et, "ITRF93", "NONE", "EARTH")
        _, el, _ = az_el(sp, "MOON", et, pos, obsctr="EARTH", obsref="ITRF93")
        if el >= DSN_MIN_EL_DEG:
            return True
    return False


def metrics(sp, lat, lon, h_m, start_utc, days, step_min=30, mask=None, panel_m2=1.0, eff=0.30):
    obspos = site_vector_km(lat, lon, h_m)
    et0 = sp.str2et(start_utc)
    n = int(days * 1440 / step_min)
    lit, dte, dsn, power, rows = [], [], [], [], []
    for i in range(n):
        et = et0 + i * step_min * 60.0
        s = sky_state(sp, et, obspos, mask)
        d = dsn_sees_moon(sp, et)
        flux = SOLAR_CONST * (AU_KM / s["sun_km"]) ** 2
        # vertical panel that tracks Sun azimuth: incidence angle = Sun elevation
        p = flux * panel_m2 * eff * s["sun_frac"] * math.cos(math.radians(s["sun_el"]))
        lit.append(s["sun_frac"]); dsn.append(d); dte.append(s["earth_vis"] and d); power.append(p)
        rows.append(dict(utc=sp.et2utc(et, "ISOC", 0), **{k: round(v, 5) if isinstance(v, float) else v
                                                           for k, v in s.items()}, dsn=d, power_w=round(p, 2)))
    dark = np.array(lit) < 0.01
    longest, run = 0, 0
    for x in dark:
        run = run + 1 if x else 0
        longest = max(longest, run)
    return dict(IlluminationPct=100 * float(np.mean(lit)), LongestDarkHours=longest * step_min / 60,
                DTEPct=100 * float(np.mean(dte)), DSNCoveragePct=100 * float(np.mean(dsn)),
                AvgPowerW=float(np.mean(power))), rows


def horizon_mask(dem, px_m, row0, col0, lon_deg, step_deg=0.25, max_km=150.0, eye_m=2.0,
                 h0_m=None, min_km=0.0):
    """Horizon elevation (deg) per azimuth from a south-polar-stereographic float DEM (metres).

    dem[row, col], row increasing = grid -y. Azimuth is measured from local north (toward
    the equator along the site's meridian), clockwise. Includes lunar curvature drop d^2/2R.
    h0_m/min_km let a coarse far-field DEM reuse the true site height and skip the near field.
    """
    lam = math.radians(lon_deg)
    north = np.array([math.sin(lam), math.cos(lam)])       # grid (x, y) direction of local north
    east = np.array([math.cos(lam), -math.sin(lam)])
    h0 = (dem[row0, col0] if h0_m is None else h0_m) + eye_m
    d = np.arange(max(px_m, min_km * 1000.0), max_km * 1000.0, 0.5 * px_m)  # half-pixel steps so thin ridges are not skipped
    drop = d * d / (2.0 * R_MOON_KM * 1000.0)
    out = np.full(int(round(360 / step_deg)), -90.0)
    for i in range(len(out)):
        a = math.radians(i * step_deg)
        v = math.cos(a) * north + math.sin(a) * east
        cols = np.rint(col0 + v[0] * d / px_m).astype(int)
        rows = np.rint(row0 - v[1] * d / px_m).astype(int)
        ok = (rows >= 0) & (rows < dem.shape[0]) & (cols >= 0) & (cols < dem.shape[1])
        if ok.any():
            el = np.degrees(np.arctan2(dem[rows[ok], cols[ok]] - h0 - drop[ok], d[ok]))
            out[i] = el.max()
    return out


def horizons_azel(target, lat, lon, start, stop, step="1 d"):
    q = dict(format="text", COMMAND=f"'{target}'", EPHEM_TYPE="OBSERVER", MAKE_EPHEM="YES",
             CENTER="'coord@301'", COORD_TYPE="'GEODETIC'", SITE_COORD=f"'{lon},{lat},0'",
             START_TIME=f"'{start}'", STOP_TIME=f"'{stop}'", STEP_SIZE=f"'{step}'",
             QUANTITIES="'4'", ANG_FORMAT="DEG", CSV_FORMAT="YES", EXTRA_PREC="YES")
    txt = urllib.request.urlopen("https://ssd.jpl.nasa.gov/api/horizons.api?" +
                                 urllib.parse.urlencode(q), timeout=60).read().decode()
    body = txt.split("$$SOE")[1].split("$$EOE")[0].strip().splitlines()
    return [(c[0].strip(), float(c[3]), float(c[4])) for c in (l.split(",") for l in body)]


def validate(sp, lat=-89.9, lon=0.0):
    obspos = site_vector_km(lat, lon)
    worst = 0.0
    for name, code in (("SUN", "10"), ("EARTH", "399")):
        for utc, h_az, h_el in horizons_azel(code, lat, lon, "2026-11-10", "2026-11-17"):
            et = sp.str2et(utc)
            az, el, _ = az_el(sp, name, et, obspos)
            daz = (az - h_az + 180) % 360 - 180
            worst = max(worst, abs(el - h_el), abs(daz) * math.cos(math.radians(el)))
            print(f"{name:5s} {utc}  ours az={az:9.4f} el={el:8.4f} | Horizons az={h_az:9.4f} "
                  f"el={h_el:8.4f} | d_el={el - h_el:+.4f} deg")
    print(f"worst angular difference: {worst:.4f} deg")
    assert worst < 0.02, "SPICE pipeline disagrees with JPL Horizons"


def selftest():
    assert disk_visible_fraction(5, 0, 0.27) == 1.0 and disk_visible_fraction(-5, 0, 0.27) == 0.0
    assert abs(disk_visible_fraction(0, 0, 0.27) - 0.5) < 1e-12
    assert abs(disk_visible_fraction(0.135, 0, 0.27) - 0.8045) < 1e-3      # half-radius above
    # 1 km-high ring wall 10 km away on flat ground -> horizon ~ atan(1/10) minus curvature
    n, px = 801, 100.0
    yy, xx = np.mgrid[0:n, 0:n]
    r = np.hypot(yy - 400, xx - 400) * px
    dem = np.where((r > 9900) & (r < 10100), 1000.0, 0.0)
    m = horizon_mask(dem, px, 400, 400, 0.0, step_deg=1.0, max_km=30, eye_m=0.0)
    expect = math.degrees(math.atan2(1000 - 10000 ** 2 / (2 * R_MOON_KM * 1000), 10000))
    assert np.all(np.abs(m - expect) < 0.4), (m.min(), m.max(), expect)
    print("selftest ok")


if __name__ == "__main__":
    ap = argparse.ArgumentParser()
    ap.add_argument("--kernels", default="kernels")
    ap.add_argument("--site", nargs=2, type=float, metavar=("LAT", "LON"))
    ap.add_argument("--height", type=float, default=0.0, help="metres above 1737.4 km sphere")
    ap.add_argument("--mask", help="horizon CSV (azimuth_deg,elevation_deg)")
    ap.add_argument("--start", default="2026-11-01T00:00:00")
    ap.add_argument("--days", type=float, default=30)
    ap.add_argument("--csv", help="write the time series here")
    ap.add_argument("--validate", action="store_true")
    ap.add_argument("--selftest", action="store_true")
    a = ap.parse_args()
    if a.selftest:
        selftest()
    else:
        sp = load_kernels(a.kernels)
        if a.validate:
            validate(sp)
        if a.site:
            mask = load_mask(a.mask) if a.mask else None
            m, rows = metrics(sp, a.site[0], a.site[1], a.height, a.start, a.days, mask=mask)
            print(json.dumps(m, indent=2))
            if a.csv:
                with open(a.csv, "w", newline="") as f:
                    w = csv.DictWriter(f, fieldnames=rows[0].keys()); w.writeheader(); w.writerows(rows)
