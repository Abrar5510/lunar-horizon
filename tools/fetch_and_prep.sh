#!/bin/sh
# Wait for the DEMs that are complete, then build grids, hero heightmaps and horizon masks from them.
#   tools/fetch_and_prep.sh            (re-run after more DEMs finish: finer data replaces coarser)
set -e
cd "$(dirname "$0")/.."
DEM=dem; READY=dem_ready
rm -rf $READY && mkdir -p $READY
while read -r URL; do
  F=$DEM/$(basename "$URL")
  WANT=$(curl -sIL "$URL" | awk 'tolower($1)=="content-length:"{n=$2} END{print n+0}')
  [ -f "$F" ] && [ "$(stat -f %z "$F")" -eq "$WANT" ] && ln -s "$(cd $DEM && pwd)/$(basename "$F")" $READY/ || echo "not ready: $(basename "$F")"
done < $DEM/urls.txt
for L in $READY/*.lbl; do [ -e "${L%.lbl}.img" ] || rm "$L"; done   # a label is only usable with its image
.venv/bin/python tools/prep_dem.py LunarHorizon/Content/Data/Sites.json $READY sites
.venv/bin/python tools/make_horizon_masks.py LunarHorizon/Content/Data/Sites.json sites LunarHorizon/Content/Data/Horizons
