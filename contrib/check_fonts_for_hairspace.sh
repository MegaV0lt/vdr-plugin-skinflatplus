#!/usr/bin/env bash

# List fonts that have the char 'Hairspace' (U+200A), almost equal to (≈) U+2248
# and average sign (Ø) (U+00D8) in their charset.

set -o nounset

# /usr/share/fonts/truetype/freefont/FreeMono.ttf: FreeMono:style=Standard,нормален,normal,obyčejné,µεσαία,Regular,Normaali,Normál,Normale,Standaard,Normalny,Обычный,Normálne,menengah,прямій,navadno,vidējs,normalusis,thường,Arrunta,सामान्य
echo -e 'Fonts with "Hairspace", "average sign" (Ø) and "almost equal" (≈):\n'
mapfile -t < <(fc-list ":charset=200A,2248,00D8" | sort -u)
for font in "${MAPFILE[@]}" ; do
  : "${font%%: *}" ; file="${_##*/}"
  : "${font##*: }" ; name="${_%%,*}"
  echo "Font: $name (${file})"  # FreeMono:style=standard (FreeMono.ttf)
done
