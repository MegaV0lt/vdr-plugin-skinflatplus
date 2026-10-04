# skinflatplus

This is a "plugin" for the Video Disk Recorder (VDR).

Written by:             Martin Schirrmacher <vdr.skinflatplus@schirrmacher.eu>

Project's homepage:     [projects.vdr-developer.org](http://projects.vdr-developer.org/projects/plg-skinflatplus/)

Project Wiki:           [projects.vdr-developer.org/wiki](http://projects.vdr-developer.org/projects/plg-skinflatplus/wiki)

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
See the file COPYING for more information.

## Anforderungen

- VDR ab Version 2.3.8 für Plugin-Version ab 1.0.0 (Plugin-Version bis 0.8.4: VDR ab 1.7.34)
- GraphicsMagick oder ImageMagick zur Anzeige von PNG/JPG-Icons, Kanal-Logos und EPG-Bildern

## Beschreibung

Skin flatPlus ist ein schneller, moderner und aktueller Skin für VDR.
Das Design ist flach und geradlinig (keine glossy- oder 3D-Effekte).
Hauptmenü mit aktivierten 'Widgets':
![grafik](https://github.com/MegaV0lt/vdr-plugin-skinflatplus/assets/2804301/6a938d74-5c32-4d05-a043-21b6a16270ba)

## git-Zugriff

Auf das Git kann mittels
`git clone https://github.com/MegaV0lt/vdr-plugin-skinflatplus.git`

zugegriffen werden.
Fehler können nicht ausgeschlossen werden ;-) (läuft aber bei mir stabil)

## Installation

Installation wie bei allen VDR-Plugins.
    make
    make install

Für die Kanallogos empfehle ich Logos aus einem der Repositories:

- <https://github.com/MegaV0lt/Picons2VDR>

Veraltet:

- <https://github.com/MegaV0lt/Picon.cz2VDR>
- <https://github.com/MegaV0lt/MP_Logos>

Die Logos sollten im gemeinsamen, skin-unabhängigen Logo-Ordner des VDR
zur Verfügung gestellt werden:
    `<vdrresdir>/logos`

Das Skin sucht Kanallogos im PNG-Format in folgenden Schreibweisen:

1. Normal wie in der Kanalliste (Nick/Comedy Channel.png)
2. In Kleinbuchstaben (nick/comedy channel.png) - Umlaute bleiben groß (ÄÖÜ)
3. In Kleinbuchstaben mit ~ (nick~comedy channel.png)

Man kann dem Skin den Pfad für die Logos beim Start mitgeben:
  `-l <LOGOPATH>`
oder
  `--logopath=<LOGOPATH>`

Das Hintergrundbild für Kanallogos (logo_background.png) kann im Kanallogo-Ordner bereitgestellt werden.
Dies ersetzt dann das Hintergrundlogo des Themas. Nützlich mit transparenten Logos.

Der Skin muss im Menü unter Einstellungen -> OSD ausgewählt werden.

## Schriften

Im Ordner `contrib/Fonts` sind verschiedene Schriften abgelegt, die einfach nach `/usr/share/fonts` kopiert werden können.
Ich empfehle DroidSans für die Anzeige zu verwenden.

## Versteckte Einstellungen

Versteckte Einstellungen sind Einstellungen, die in der VDR setup.conf konfiguriert werden können, wozu es aber keine Einstellungen im OSD -> Einstellungen -> Plugins -> skinflatplus gibt.

* MenuItemRecordingClearPercent - Wenn die Einstellung auf 1 gesetzt ist, wird vom Aufnahmetext das Prozentzeichen am Anfang des Strings entfernt.
* MenuItemRecordingShowFolderDate - Wenn die Einstellung auf 1 gesetzt ist, wird bei einem Ordner von der neuesten Aufzeichnung das Datum angezeigt. Wenn die Einstellung auf 2 gesetzt ist, wird bei einem Ordner von der ältesten Aufzeichnung das Datum angezeigt.
* MenuItemParseTilde - Wenn die Einstellung auf 1 gesetzt ist, wird beim Menü-Item-Text auf den Buchstaben Tilde '~' geprüft und wenn eine Tilde gefunden wurde, wird die Tilde entfernt und alles, was nach der Tilde steht, in einer anderen Farbe dargestellt. Dies ist z.B. interessant, wenn man EPGSearch hat.

## Widgets

Seit Version 0.5.0 gibt es die Widgets.
Es gibt interne und externe Widgets. Die internen Widgets funktionieren out-of-the-box, da sie innerhalb des VDR implementiert sind.
Externe Widgets sind externe Programme/Scripte; hier sind teilweise manuelle Anpassungen notwendig, damit diese laufen.

Die Widgets werden auf der rechten Seite des Hauptmenüs angezeigt. In der Höhe ist nur begrenzt Platz, deshalb kann es passieren, dass nicht alle Widgets angezeigt werden können (da einfach kein Platz mehr ist). Hier musst du selbst entscheiden, welches Widget auf welcher Position angezeigt werden soll.

## Interne Widgets

### DVB-Geräte

Zeigt die DVB-Geräte an, wer diese benutzt und auf welchem Kanal das Gerät gerade ist. Über die Plugineinstellungen kann konfiguriert werden, ob "unbekannte" und/oder "nicht benutzte" Geräte ausgeblendet werden sollen. Die Nummerierung der Geräte kann entweder wie VDR-intern mit 0 beginnen oder wahlweise mit 1.
Leider ist es derzeit nicht möglich, 100 % herauszufinden, wer das Gerät derzeit nutzt. Z.B. gibt es Fälle wie den EPG-Scan, der nicht erkannt wird.

### Aktive Timer

Zeigt die aktiven Timer an. Über die Plugineinstellungen kann die max. Anzahl der Timer konfiguriert werden, welche angezeigt wird. Weiter kann konfiguriert werden, dass das Widget ausgeblendet wird, wenn keine aktiven Timer existieren.

### Timer-Konflikte

Zeigt die Anzahl der Timer-Konflikte an. Es ist in Planung, dass auch die eigentlichen Konflikte und nicht nur die Anzahl angezeigt werden. Es kann wieder konfiguriert werden, dass das Widget ausgeblendet wird, wenn keine Timer-Konflikte vorhanden sind.

### Letzte Aufnahmen

Zeigt die letzten Aufnahmen nach Datum sortiert an. Über die Plugineinstellungen kann die max. Anzahl der Elemente konfiguriert werden.

## Externe Widgets

Externe Scripte werden (wenn nicht anders konfiguriert) in das LIBDIR installiert. Normalerweise sollte dies folgender Pfad sein:
`/usr/local/lib/vdr/skinflatplus/widgets/`

Alle Ausgaben der Scripte sind unter `/tmp/skinflatplus/widgets/`

### Wetter-Widget

Zeigt das aktuelle Wetter und eine Vorschau an. Die Ansicht der Vorschau kann konfiguriert werden. Es existiert eine Lang- und eine Kurzansicht. Bei der langen Ansicht gibt es pro Tag eine Zeile; bei der kurzen Ansicht wird alles in einer Zeile dargestellt. Die Anzahl der Tage kann über die Plugineinstellungen konfiguriert werden, wobei max. 7 Tage möglich sind.
Dieses Widget benötigt jq; unter Ubuntu ist z.B. das Paket jq notwendig.
Im Ordner existiert eine update_weather.conf.dist; diese muss nach update_weather.conf kopiert werden.
`cd /usr/local/lib/vdr/skinflatplus/widgets/weather

cp update_weather.conf.dist update_weather.conf`
Anschließend muss Latitude und Longitude vom Ort ermittelt werden; dafür gibt es z.B. die Seite <https://www.latlong.net>.
Die Werte von Latitude und Longitude in die update_weather.conf schreiben und auch den Wert "LOCATION" entsprechend anpassen. "LOCATION" wird später im Skin als Ort angezeigt.

Das Script (update_weather.sh) wird nicht vom Skin aufgerufen. Dies muss extern über cron oder ähnliches erfolgen. Z.B. über folgende Zeile in der /etc/crontab

```pre
# Update weather every hour
@hourly  root  /usr/bin/bash /usr/local/lib/vdr/skinflatplus/widgets/weather/update_weather.sh
```

Für die Wetterdaten wird openweathermap.org verwendet. Hier sind 1.000 Abfragen am Tag (30.000 im Monat) frei (<https://openweathermap.org/full-price#current>). Die Registrierung ist kostenlos, und man kann einen eigenen API-Key erstellen. Diesen dann einfach in die update_weather.conf eintragen. Hierfür ist nur eine E-Mail-Adresse + Passwort notwendig.

Für die Kanalinfo gibt es eine kleine Version des Wetter-Widgets. Hier wird Heute + Morgen angezeigt.

### System-Informationen

Mit diesem Widget können verschiedene Systeminformationen angezeigt werden.
Dieses Script wird bei jedem Aufruf des Menüs erneut ausgeführt. Daher sollte das Script kurz und schnell sein.
Es wird das Script "system_information" ausgeführt. Dieses muss manuell verlinkt werden! Für Ubuntu z.B. wie folgt

```pre
cd /usr/local/lib/vdr/skinflatplus/widgets/system_information
ln -s system_information.ubuntu system_information
```

In dem Script ist auch die Konfiguration enthalten. Hier kann festgelegt werden, welche Informationen ausgegeben werden sollen und in welcher Position diese sein sollen.

### Temperaturen

Mit diesem Widget können die Temperaturen des Systems angezeigt werden.
Dieses Script wird bei jedem Aufruf des Menüs erneut ausgeführt. Daher sollte das Script kurz und schnell sein.
Es wird das Script "temperatures" ausgeführt. Dieses muss manuell verlinkt werden! Z.B. wie folgt

```pre
cd /usr/local/lib/vdr/skinflatplus/widgets/temperatures
ln -s temperatures.default temperatures
```

Das Standard-Script nutzt lm-sensors, um die Temperaturen zu ermitteln. Du musst sicherstellen, dass du lm-sensors installiert und auch richtig konfiguriert hast. Weiterhin wird die GPU-Temperatur mittels nvidia-settings ermittelt. Dies funktioniert natürlich nur mit Nvidia-Grafikkarten.

### System-Updatestatus

Mit diesem Widget können die Updates des Systems angezeigt werden. Dabei wird die Anzahl der Updates und der Sicherheitsupdates angezeigt.
Das Script (system_update_status) wird nicht vom Skin aufgerufen. Dies muss extern über cron oder ähnliches erfolgen. Z.B. über folgende Zeile in der /etc/crontab

```pre
# update system_updates every 12 hours
7 */12   * * *   root    /usr/local/lib/vdr/skinflatplus/widgets/system_updatestatus/system_updatestatus
```

### Benutzerdefinierte Ausgabe

Mit diesem Widget können eigene Befehle ausgeführt und angezeigt werden.
Wenn im Ordner das Script "command" vorhanden ist, wird dieses bei jedem Aufruf des Menüs ausgeführt.
Das Script command muss 2 Dateien bereitstellen: title & output
Jeweils für den Titel und die eigentliche Ausgabe.
Es sollte darauf geachtet werden, dass nicht zu viele Zeilen ausgegeben werden, da immer die gesamte Datei angezeigt wird. Die Zeilen werden rechts einfach abgeschnitten und nicht umgebrochen!

## TVScraper & scraper2vdr

Seit Version 0.3.0 unterstützt der Skin TVScraper & scraper2vdr.
Mit beiden Plugins erhält man Poster-, Banner- und Schauspielerbilder für Aufnahmen und EPG-Informationen.

## epgd & doppelte Informationen im EPG-Text

Wenn epgd + epg2vdr verwendet wird, wird der angezeigte EPG-Text über die eventsview.sql festgelegt (in der epgd.conf-Option: EpgView).
Mit der Standard-eventsview.sql ist im EPG-Text die Schauspieler-, Serien- und Filminformation enthalten. Da diese dann doppelt angezeigt werden würden (im EPG-Text und in den extra Bereichen über scraper2vdr), existiert im contrib-Ordner von flatPlus eine eigene "eventsview-flatplus.sql". Mit dieser wird im EPG-Text wirklich nur der EPG-Text ausgegeben und keine weiteren Informationen.
Ich empfehle diese zu verwenden. Dafür einfach die Datei aus dem contrib-Ordner nach /etc/epgd/ kopieren und in der epgd.conf folgenden Eintrag verwenden:

EpgView = eventsview-flatplus.sql

Wenn externes EPG verwendet wird, empfehle ich:

EpgView = eventsview-MV.sql

## Themen und Themen-spezifische Icons

Der Skin ist weitestgehend über Themes anpassbar.
Die Decorations (Border, ProgressBar) sind über das Theme einstellbar. Dabei kann jeweils der Typ und
die Größe (in Pixeln) eingestellt werden. Dabei wird von dem ARGB im Theme nur B verwendet. Es muss darauf geachtet werden,
dass die Werte in Hex angegeben werden. Wenn man also z.B. eine Größe von 20 Pixeln angeben möchte, heißt der Wert: 00000014.
Siehe dazu die Beispiele.

```pre
Borders:
    0 = none
    1 = rect
    2 = round
    3 = invert round
    4 = rect + alpha blend
    5 = round + alpha blend
    6 = invert round + alpha blend
Beispiel:
    clrChannelBorderType = 00000004
    clrChannelBorderSize = 0000000F

ProgressBar:
    0 = small line + big line
    1 = big line
    2 = big line + outline
    3 = small line + big line + dot
    4 = big line + dot
    5 = big line + outline + dot
    6 = small line + dot
    7 = outline + dot
    8 = small line + big line + alpha blend
    9 = big line + alpha blend
Beispiel:
    clrChannelProgressType = 00000008
    clrChannelProgressSize = 0000000F
```
