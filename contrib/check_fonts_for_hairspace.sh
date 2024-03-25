#!/usr/bin/env bash

# List fonts that have the char 'Hairspace'

# /usr/share/fonts/truetype/freefont/FreeMono.ttf: FreeMono:style=Standard,нормален,normal,obyčejné,µεσαία,Regular,Normaali,Normál,Normale,Standaard,Normalny,Обычный,Normálne,menengah,прямій,navadno,vidējs,normalusis,thường,Arrunta,सामान्य
echo -e 'Schriften, die das Zeichen "HairPace" enthalten:\n'
mapfile -t < <(fc-list ":charset=0x200A" | sort -u)
for font in "${MAPFILE[@]}" ; do
  : "${font%%: *}" ; file="${_##*/}"
  : "${font##*: }" ; name="${_%%,*}"
  echo "Font: $name (${file})"  # FreeMono:style=standard (FreeMono.ttf)
done
