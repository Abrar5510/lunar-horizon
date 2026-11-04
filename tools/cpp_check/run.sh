#!/bin/sh
# Build the LHScience C++ core without Unreal and check it against tools/lunar_sky.py.
#   tools/cpp_check/run.sh [SITE_ID ...]     (default: REF_POLE; masked sites need Horizons/<Id>.csv)
set -e
cd "$(dirname "$0")/../.."
PY=${PY:-.venv/bin/python}
K=LunarHorizon/Content/SPICE
D=LunarHorizon/Content/Data
S=LunarHorizon/Source
OUT=${TMPDIR:-/tmp}/lh_cpp_check
mkdir -p "$OUT"
clang++ -std=c++17 -O2 -DLH_STANDALONE -I$S/LHScience/Public -I$S/LHScience/Private -I$S/ThirdParty/CSPICE/include \
  tools/cpp_check/check.cpp $S/LHScience/Private/LHSpice.cpp $S/ThirdParty/CSPICE/lib/Mac/libcspice.a -o "$OUT/check"
for ID in ${@:-REF_POLE}; do
  set -- $($PY -c "import json,sys; s=[s for s in json.load(open('$D/Sites.json')) if s['Id']=='$ID'][0]; print(s['LatDeg'], s['LonDeg'], s['HeightM'])")
  MASK=$D/Horizons/$ID.csv
  [ -f "$MASK" ] || MASK=""
  echo "== $ID lat=$1 lon=$2 h=$3 mask=${MASK:-none}"
  $PY tools/lunar_sky.py --kernels $K --site $1 $2 --height $3 ${MASK:+--mask $MASK} \
    --start 2026-11-14T00:00:00 --days 30 --csv "$OUT/$ID.csv"
  "$OUT/check" $K "$OUT/$ID.csv" $1 $2 $3 $MASK
done
