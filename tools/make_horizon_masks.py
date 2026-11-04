"""prep_dem.py grids -> Content/Data/Horizons/<Id>.csv (2 m eye) and <Id>_10m.csv (lander mast).

  python tools/make_horizon_masks.py LunarHorizon/Content/Data/Sites.json sites LunarHorizon/Content/Data/Horizons

Near field: 5 m grid out to 12 km. Far field: 60 m grid from 12 km to 150 km, using the true site
height from the 5 m grid (a coarse DEM smooths peaks and pits, which would bias h0).
"""
import json, os, sys
import numpy as np
from lunar_sky import horizon_mask

NEAR_KM, FAR_KM = 12.0, 150.0


def main(sites_json, grids, out_dir):
    os.makedirs(out_dir, exist_ok=True)
    for s in json.load(open(sites_json)):
        if not os.path.exists(f"{grids}/{s['Id']}_near5m.npy"):
            print(f"{s['Id']:16s} SKIPPED: no grids (prep_dem.py had no DEM for it)")
            continue
        # gaps (outside every DEM) become a deep pit, so they never form the horizon
        near = np.nan_to_num(np.load(f"{grids}/{s['Id']}_near5m.npy"), nan=-1e4)
        far = np.nan_to_num(np.load(f"{grids}/{s['Id']}_far60m.npy"), nan=-1e4)
        cn, cf = near.shape[0] // 2, far.shape[0] // 2
        h0 = float(near[cn, cn])
        for eye, suffix in ((2.0, ""), (10.0, "_10m")):
            m = np.maximum(horizon_mask(near, 5.0, cn, cn, s["LonDeg"], max_km=NEAR_KM, eye_m=eye),
                           horizon_mask(far, 60.0, cf, cf, s["LonDeg"], max_km=FAR_KM, eye_m=eye,
                                        h0_m=h0, min_km=NEAR_KM))
            np.savetxt(f"{out_dir}/{s['Id']}{suffix}.csv", np.c_[np.arange(len(m)) * 0.25, m],
                       delimiter=",", header="azimuth_deg,elevation_deg", comments="", fmt="%.2f,%.3f")
        print(f"{s['Id']:16s} horizon {m.min():6.2f} .. {m.max():6.2f} deg (10 m mast)")


if __name__ == "__main__":
    main(*sys.argv[1:4])
