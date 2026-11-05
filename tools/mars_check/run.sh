#!/bin/sh
# Build MarsCore.h with clang (no Unreal), check DEM + router, and compare the Mars24 sun with SPICE.
set -e
cd "$(dirname "$0")/../.."
PY=${PY:-.venv/bin/python}
OUT=${TMPDIR:-/tmp}/mars_check
mkdir -p "$OUT"
clang++ -std=c++17 -O2 -IMarsMap/Source/MarsMap tools/mars_check/check.cpp -o "$OUT/check"
TIMES=$($PY -c "print(' '.join(str(1767225600 + i * 86400 * 23) for i in range(100)))")   # 2026-01-01 + 6 years
"$OUT/check" MarsMap/Content/Data $TIMES > "$OUT/sun.csv"
$PY - "$OUT/sun.csv" <<'EOF'
import csv, math, sys
from datetime import datetime, timezone
import spiceypy as sp
for k in ("naif0012.tls", "pck00011.tpc", "de440s.bsp", "mar099s.bsp"):
    sp.furnsh("kernels/" + k)
re, _, rp = sp.bodvrd("MARS", "RADII", 3)[1]
f2 = (rp / re) ** 2
worst = [0, 0, 0]
for r in csv.DictReader(open(sys.argv[1])):
    et = sp.str2et(datetime.fromtimestamp(float(r["unix"]), timezone.utc).strftime("%Y-%m-%dT%H:%M:%S"))
    v, _ = sp.spkpos("SUN", et, "IAU_MARS", "LT+S", "MARS")
    _, lon, lat = sp.reclat(v)
    lat_g = math.degrees(math.atan(math.tan(lat) / f2))      # planetocentric -> planetographic
    lon_e = math.degrees(lon) % 360
    ls = math.degrees(sp.lspcn("MARS", et, "LT+S"))
    dlon = (float(r["sublon"]) - lon_e + 180) % 360 - 180
    dls = (float(r["ls"]) - ls + 180) % 360 - 180
    for i, d in enumerate((dls, dlon, float(r["sublat"]) - lat_g)):
        worst[i] = max(worst[i], abs(d))
print("Mars24 vs SPICE, worst |error| deg: Ls %.4f  subsolar lon %.4f  subsolar lat %.4f" % tuple(worst))
assert worst[0] < 0.05 and worst[1] < 0.2 and worst[2] < 0.05, "Mars24 disagrees with SPICE"
print("OK")
EOF
