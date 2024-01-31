#!/usr/bin/env bash

#
# update_weather.sh
#
# Skript zum laden von Wetterdaten für Skin FlatPlus
# Daten werden von openweathermap.org geladen. Dazu wird ein API-Key benötigt,
# der kostenlos bezogen werden kann (https://openweathermap.org/price)
#
# Wettersymbole von harmattan: https://github.com/zagortenay333/Harmattan
# Das Skript verwendet 'jq' zum verarbeiten der .json-Dateien
#
# Einstellungen zum Skript in der dazugehörigen *.conf vornehmen!
#
#VERSION=240129

### Variablen ###
SELF="$(readlink /proc/$$/fd/255)" || SELF="$0"  # Eigener Pfad (besseres $0)
SELF_NAME="${SELF##*/}"                          # skript.sh
CONF="${SELF%.*}.conf"                           # Konfiguration
DATA_DIR='/tmp/skinflatplus/widgets/weather'     # Verzeichnis für die Daten
WEATHER_JSON="${DATA_DIR}/weather.json"          # Aktuelles Wetter
LC_NUMERIC='C'
API_URL='https://api.openweathermap.org/data/2.5/onecall'  # API 3.0 benötigt extra Abo (1.000 Abrufe/Tag frei)

### Funktionen ###
f_log(){
  if [[ -t 1 ]] ; then
    echo "$*"
  else
    logger -t "$SELF_NAME" "$*" 2>/dev/null
  fi
}

f_write_temp(){  # Temperaturwert aufbereiten und schreiben ($1 Temperatur, $2 Ausgabedatei)
  local data="$1" file="$2"
  printf -v data '%.1f' "$data"                  # Temperatur mit einer Nachkommastelle
  [[ -n "$DEC" ]] && data="${data/./"$DEC"}"     # . durch , ersetzen
  printf '%s' "${data}${DEGREE_SIGN}" > "$file"  # Daten schreiben (13,1°C)
}

f_get_weather(){
  local jqdata jqdata2

  # Wetterdaten laden (One Call API)
  curl --silent --retry 5 --retry-delay 15 --output "$WEATHER_JSON" \
    "${API_URL}?lat=${LATITUDE}&lon=${LONGITUDE}&exclude=minutely,hourly,alerts&units=${UNITS}&lang=${L}&appid=${API_KEY}"

  [[ ! -e "$WEATHER_JSON" ]] && { f_log "Download of $WEATHER_JSON failed!" ; exit 1 ;}

  # Aktuelle Wetterdaten
  printf '%s\n' "$LOCATION" > "${DATA_DIR}/weather.location"       # Der Ort für die Werte z. B. Berlin
  jqdata=$(jq -r .current.temp "$WEATHER_JSON")                    # Aktuelle Temperatur
  f_write_temp "$jqdata" "${DATA_DIR}/weather.0.temp"
  jqdata2=$(jq -r .current.weather[0].description "$WEATHER_JSON") # Beschreibung (Aktuell)
  jqdata=$(jq -r .current.weather[0].id "$WEATHER_JSON")           # Wettersymbol (ID)
  case "$jqdata" in
    800) [[ $(jq -r .current.weather[0].icon "$WEATHER_JSON") =~ n ]] && jqdata='clear-night' ;;
    801) [[ $(jq -r .current.weather[0].icon "$WEATHER_JSON") =~ n ]] && jqdata='partly-cloudy-night' ;;
    *) ;;
  esac
  printf '%s\n' "$jqdata" > "${DATA_DIR}/weather.0.icon-act"       # Symbol für 'Jetzt'

  # x-Tage Vorhersage
  while [[ ${cnt:=0} -lt $FORECAST_DAYS ]] ; do
    jqdata=$(jq -r .daily[${cnt}].temp.night "$WEATHER_JSON")      # Temperatur (Min./Nacht)
    f_write_temp "$jqdata" "${DATA_DIR}/weather.${cnt}.tempMin"
    jqdata=$(jq -r .daily[${cnt}].temp.day "$WEATHER_JSON")        # Temperatur (Max./Tag)
    f_write_temp "$jqdata" "${DATA_DIR}/weather.${cnt}.tempMax"
    jq -r .daily[${cnt}].pop "$WEATHER_JSON" \
      > "${DATA_DIR}/weather.${cnt}.precipitation"                 # Niederschlagswahrscheinlichkeit
    jqdata=$(jq -r .daily[${cnt}].weather[0].description "$WEATHER_JSON")
    if [[ "$cnt" -eq 0 ]] ; then
      [[ "$jqdata" != "$jqdata2" ]] && jqdata+=" / $jqdata2"       # Beschreibung / Aktuell
    fi
    printf '%s\n' "$jqdata" > "${DATA_DIR}/weather.${cnt}.summary" # Beschreibung
    jqdata=$(jq -r .daily[${cnt}].weather[0].id "$WEATHER_JSON")   # Wettersymbol
    printf '%s\n' "$jqdata" > "${DATA_DIR}/weather.${cnt}.icon"
    ((cnt++))
  done
}

### Start ###
# Datenverzeichnis erstellen
[[ ! -d "$DATA_DIR" ]] && { mkdir --parents "$DATA_DIR" || exit 1 ;}

rm "${DATA_DIR}/weather.*" &>/dev/null  # Alte Daten löschen

# Konfiguration laden und prüfen
[[ -e "$CONF" ]] && { source "$CONF" || exit 1 ;}
for var in API_KEY FORECAST_DAYS _LANG LOCATION UNITS ; do
  [[ -z "${!var}" ]] && { f_log "Error: $var is not set!" ; RC=1 ;}
done
[[ -n "$RC" ]] && exit 1

L="${_LANG%%_*}"  # Sprache für den Abruf via Openweathermap-API (de)

# Temperatureinheit (Standard [°K], Metric [°C], Imperial [°F])
case "$UNITS" in
  metric)  DEGREE_SIGN='°C' ; DEC=',' ;;
  imperal) DEGREE_SIGN='°F' ;;
  *)       DEGREE_SIGN='°K' ;;
esac

f_get_weather  # Wetterdaten holen und aufbereiten

exit
