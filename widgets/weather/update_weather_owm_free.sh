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
#VERSION=240226

### Variablen ###
SELF="$(readlink /proc/$$/fd/255)" || SELF="$0"  # Eigener Pfad (besseres $0)
SELF_NAME="${SELF##*/}"                          # skript.sh
CONF="${SELF%.*}.conf"                           # Konfiguration
DATA_DIR='/tmp/skinflatplus/widgets/weather'     # Verzeichnis für die Daten
CUR_WEATHER_JSON="${DATA_DIR}/weather_cur.json"  # Aktuelles Wetter
WEATHER_JSON="${DATA_DIR}/weather.json"          # Aktuelles Wetter
LC_NUMERIC='C'
CUR_API_URL='https://api.openweathermap.org/data/2.5/weather'  # API 3.0 benötigt extra Abo (1.000 Abrufe/Tag frei)
API_URL='https://api.openweathermap.org/data/2.5/forecast'  # API 3.0 benötigt extra Abo (1.000 Abrufe/Tag frei)
FORECAST_EVENTS_DAY='8'

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

  # Current weather (weather API)
  curl --silent --retry 5 --retry-delay 15 --output "$CUR_WEATHER_JSON" \
    "${CUR_API_URL}?lat=${LATITUDE}&lon=${LONGITUDE}&units=${UNITS}&lang=${L}&appid=${API_KEY}"

  [[ ! -e "$CUR_WEATHER_JSON" ]] && { f_log "Download of $CUR_WEATHER_JSON failed!" ; exit 1 ;}

  # Aktuelle Wetterdaten
  printf '%s\n' "$LOCATION" > "${DATA_DIR}/weather.location"       # Der Ort für die Werte z. B. Berlin
  jqdata=$(jq -r .main.temp "$CUR_WEATHER_JSON")                    # Aktuelle Temperatur
  f_write_temp "$jqdata" "${DATA_DIR}/weather.0.temp"
  jqdata2=$(jq -r .weather[0].description "$CUR_WEATHER_JSON") # Beschreibung (Aktuell)
  printf '%s\n' "$jqdata2" > "${DATA_DIR}/weather.0.summary" # Beschreibung
  jqdata=$(jq -r .weather[0].id "$CUR_WEATHER_JSON")           # Wettersymbol (ID)
  case "$jqdata" in
    800) [[ $(jq -r .weather[0].icon "$CUR_WEATHER_JSON") =~ n ]] && jqdata='clear-night' ;;
    801) [[ $(jq -r .weather[0].icon "$CUR_WEATHER_JSON") =~ n ]] && jqdata='partly-cloudy-night' ;;
    *) ;;
  esac
  printf '%s\n' "$jqdata" > "${DATA_DIR}/weather.0.icon-act"       # Symbol für 'Jetzt'

  # Forecast weather (forecast API)
  curl --silent --retry 5 --retry-delay 15 --output "$WEATHER_JSON" \
  "${API_URL}?lat=${LATITUDE}&lon=${LONGITUDE}&units=${UNITS}&lang=${L}&appid=${API_KEY}"

  [[ ! -e "$WEATHER_JSON" ]] && { f_log "Download of $WEATHER_JSON failed!" ; exit 1 ;}
  # Todays date
  datetoday=$(date +"%Y-%m-%d")
  daycnt=0
  totalevents=$(( $FORECAST_DAYS * $FORECAST_EVENTS_DAY ))

  # x-Tage Vorhersage
  while [[ ${cnt:=0} -lt $totalevents ]] ; do
    jqdata=$(jq -r .list[${cnt}].dt_txt "$WEATHER_JSON")      # timestamp
    fctime=( $jqdata )
      if [[ "${fctime[1]}" == "03:00:00" ]] ; then                   # Night temperature
        jqdata=$(jq -r .list[${cnt}].main.temp "$WEATHER_JSON")    # Temperatur (Min./Nacht)
        f_write_temp "$jqdata" "${DATA_DIR}/weather.${daycnt}.tempMin"
      fi 
      if [[ "${fctime[1]}" == "15:00:00" ]] ; then                   # Day temperature
        jqdata=$(jq -r .list[${cnt}].main.temp "$WEATHER_JSON")        # Temperatur (Max./Tag)
        f_write_temp "$jqdata" "${DATA_DIR}/weather.${daycnt}.tempMax"
        jqdata=$(jq -r .list[${cnt}].weather[0].id "$WEATHER_JSON")   # Wettersymbol
        printf '%s\n' "$jqdata" > "${DATA_DIR}/weather.${daycnt}.icon"
        jq -r .list[${cnt}].pop "$WEATHER_JSON" \
          > "${DATA_DIR}/weather.${daycnt}.precipitation"                 # Niederschlagswahrscheinlichkeit
        jqdata=$(jq -r .list[${cnt}].weather[0].description "$WEATHER_JSON")
	printf '%s\n' "$jqdata" > "${DATA_DIR}/weather.${daycnt}.summary" # Beschreibung
	((daycnt++))
      fi
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
