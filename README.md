This is a "plugin" for the Video Disk Recorder (VDR).

Written by:                  Martin Schirrmacher <vdr.skinflatplus@schirrmacher.eu>

Project's homepage:          http://projects.vdr-developer.org/projects/plg-skinflatplus/

Projekt Wiki                 http://projects.vdr-developer.org/projects/plg-skinflatplus/wiki

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
See the file COPYING for more information.


Anforderungen
-------------------------------
- VDR ab Version 1.7.34
- GraphicsMagick oder ImageMagick zur Anzeige von png/jpg Icons, Kanal-Logos and EPG Bilder


Beschreibung
-------------------------------
Skin flatPlus ist ein schneller, moderner und aktueller Skin für VDR.
Das Design ist flach und geradlinig (keine glossy oder 3D-Effekte)


git-Zugriff
-------------------------------
Auf das git kann mittels

git clone https://github.com/MegaV0lt/vdr-plugin-skinflatplus.git/

zugegriffen werden.
Fehler können nicht ausgeschlossen werden ;-) (Läuft aber bei mir stabil)


Installation
-------------------------------
Installation wie bei allen VDR Plugins.
    make
    make install

Für die Kanallogos empfehle ich Logos aus einem der Repositories:
https://github.com/MegaV0lt/Picon.cz2VDR
https://github.com/MegaV0lt/MP_Logos
https://github.com/MegaV0lt/Picons2VDR

Die Logos müssen im folgenden Ordner zur Verfügung gestellt werden:
    <vdrconfigdir>/plugins/skinflatplus/logos/

Der Skin muss im Menü unter Einstellungen -> OSD ausgewählt werden.

Schriften
-------------------------------
Im Ordner contrib/Fonts sind vershiedene Schriften abgelegt, welche einfach nach /user/share/fonts kopiert werden können.
Ich empfehle DroidSans für dei anzeige zu verwenden.

Versteckte Einstellungen
-------------------------------
Versteckte Einstellungen sind Einstellungen die in der VDR setup.conf konfiguriert werden können, wozu es aber keine Einstellungen im OSD -> Einstellungen -> Plugins -> skinflatplus gibt.

* MenuItemRecordingClearPercent - Wenn die Einstellung auf 1 gesetzt ist, wird vom Aufnahmetext das Prozentzeichen am Anfang des Strings entfernt.
* MenuItemRecordingShowFolderDate - Wenn die Einstellung auf 1 gesetzt ist, wird bei einem Ordner von der neuesten Aufzeichnung das Datum angezeigt, Wenn die Einstellung auf 2 gesetzt ist, wird bei einem Ordner von der ältesten Aufzeichnung das Datum angezeigt.
* MenuItemParseTilde - Wenn die Einstellung auf 1 gesetzt ist, wird beim Menü-Item-Text auf den Buchstaben Tilde '~' geprüft und wenn eine Tilde gefunden wurde, wird die Tilde entfernt und alles was nach der Tilde steht in einer anderen Farbe dargestellt. Dies ist z.B. interessant wenn man epgsearch hat.

Widgets
_______________________________
Seit Version 0.5.0 gibt es die Widgets.
Es gibt interne und externe Widgets. Die internen Widgets funktionieren out-of-the-box, da Sie innerhalb des VDR implementiert sind.
Externe Widgets sind externe Programme/Scripte, hier sind (teilweise) manuelle Anpassungen notwendig damit diese laufen.

Die Widgets werden auf der rechten Seite des Hauptmenüs angezeigt. In der Höhe ist nur begrenzt Platz, deshalb kann es passieren das nicht alle Widgets angezeigt werden können (da einfach kein Platz mehr ist). Hier muss du selbst entscheiden welches Widget auf welcher Position angezeigt werden soll.

* Interne Widgets

** DVB Geräte

Zeigt die DVB Geräte an, wer dieses benutzt und auf welchem Kanel das Gerät gerade ist. Über die Plugineinstellungen kann konfiguriert werden ob "unbekannte" und/oder "nicht benutzte" Geräte ausgeblendet werden sollen. Die Nummerierung der Geräte kann entweder wie VDR-Intern mit 0 beginnen oder wahlweise mit 1.
Leider ist es derzeit nicht möglich 100% herauszufinden wer das Gerät derzeit nutzt. Z.B. gibt es Fälle wie den EPG-Scan welcher nicht erkannt wird.

** Aktive Timer

Zeit die aktiven Timer an. Über die Plugineinstellungen kann die max. Anzahl der Timer konfiguriert werden welche angezeigt wird. Weiter kann konfiguriert werden, dass das Widget ausgeblendet wird, wenn keine aktiven Timer existieren.

** Timer Konflikte

Zeigt die Anzahl der Timer-Konflikte an. Es ist in Plannung das auch die eigentlichen Konflikte und nicht nur die Anzahl angezeigt wird. Es kann wieder konfiguriert werden das das Widget ausgeblendet wird wenn keine Timer Konflikte vorhanden sind.

** Letzte Aufnahmen

Zeigt die letzten Aufnahmen nach Datum sortiert an. Über die Plugineinstellungen kann die max. Anzahl der Elemente konfiguriert werden.

* Externe Widgets

Externe Scripte werden (wenn nicht anders konfiguriert) in das LIBDIR installiert. Normalerweise sollte dies folgender Pfad sein.
<pre>
/usr/local/lib/vdr/skinflatplus/widgets/
</pre>

Alle Ausgaben der Scripte sind unter
<pre>
/tmp/skinflatplus/widgets/
</pre>

** Wetter-Widget

Zeigt das aktuelle Wetter und eine Vorschau an. Die Ansicht der Vorschau kann konfiguriert werden. Es existiert eine Lang- und eine Kurzansicht. Bei der langen Ansicht gibt es pro Tag eine Zeile bei der kurzen Ansicht wird alles in einer Zeile dargestellt. Die Anzahl der Tage kann über die Plugineinstellungen konfiguriert werden, wobei max. 7 Tage möglich sind.
Dieses Widget benötigt jq, unter Ubuntu ist z.B. das Paket jq notwendig.
Im Ordner existiert eine update_weather.conf.dist diese muss nach update_weather.conf kopiert werden.
<pre>
cd /usr/local/lib/vdr/skinflatplus/widgets/weather
cp update_weather.conf.dist update_weather.conf
</pre>
Anschließend muss Latitude und Longitude vom Ort ermittelt werden, dafür gibt es z. B. die Seite https://www.latlong.net.
Die Werte von Latitude und Longitude in die update_weather.conf schreiben und auch den Wert "LOCATION" entsprechend anpassen. "LOCATION" wird später im Skin als Ort angezeigt.

Das Script (update_weather.sh) wird nicht vom Skin aufgerufen. Dies muss extern über cron oder ähnliches erfolgen. Z.B. über folgende Zeile in der /etc/crontab
<pre>
# update weather every 2 hours
7 */2   * * *   root    bash /usr/local/lib/vdr/skinflatplus/widgets/weather/update_weather.sh
</pre>

Für die Wetterdaten wird openweathermap.org verwendet. Hier sind 1.000 Abfragen am Tag (30.000 im Monat) frei (https://openweathermap.org/full-price#current). Bitte registriere dich kostenlos bei openweathermap.org und erstelle einen eigenen Api-Key. Diesen dann einfach in die update_weather.conf eintragen. Hierfür ist nur eine E-Mail Adresse + Passwort notwendig.

Für die Kanalinfo gibt es eine kleine Version des Wetter Widgets. hier wird Heute + Morgen angezeigt.

** System-Informationen

Mit diesem Widget können verschiedenene Systeminformationen angezeigt werden.
Dieses Script wird bei jedem Aufruf des Menü erneut ausgeführt. Daher sollte das Script kurz und schnell sein.
Es wird das Script "system_information" ausgeführt. Dieses muss manuell verlinkt werden! Für Ubuntu z.B. wie folgt
<pre>
cd /usr/local/lib/vdr/skinflatplus/widgets/system_information
ln -s system_information.ubuntu system_information
</pre>

In dem Script ist auch die Konfiguration enthalten. Hier kann festgelegt werden welche Informationen ausgegeben werden sollen und in welcher Position diese sein sollen.

** Temperaturen

Mit diesem Widget können die Temperaturen des System angezeigt werden.
Dieses Script wird bei jedem Aufruf des Menü erneut ausgeführt. Daher sollte das Script kurz und schnell sein.
Es wird das Script "temperatures" ausgeführt. Dieses muss manuell verlinkt werden! Z.B. wie folgt
<pre>
cd /usr/local/lib/vdr/skinflatplus/widgets/temperatures
ln -s temperatures.default temperatures
</pre>

Das default Script nutzt lm-sensors um die Temeperaturen zu ermitteln. Du musst sicherstellen das du lm-sensors installiert und auch richtig konfiguriert hast. Weiterhin wird die GPU-Temperatur mittels nvidia-settings ermittelt. Dies funktioniert natürlich nur mit Nvidia Grafikkarten.

** System-Updatestatus

Mit diesem Widget können die Updates des System angezeigt werden. Dabei wird die Anzahl der Updates und der Sicherheitsupdates angezeigt.
Das Script (system_update_status) wird nicht vom Skin aufgerufen. Dies muss extern über cron oder ähnliches erfolgen. Z.B. über folgende Zeile in der /etc/crontab
<pre>
# update system_updates every 12 hours
7 */12   * * *   root    /usr/local/lib/vdr/skinflatplus/widgets/system_updatestatus/system_updatestatus
</pre>

** Benutzerdefinierte Ausgabe

Mit diesem Widget können eigene Befehle ausgeführt und angezeigt werden.
Wenn es in dem Ordner das Script "command" gibt, wird dieses bei jedem Aufruf des Menüs ausgeführt.
Das Script command muss 2 Dateien bereitstellen: title & output
Jeweils für den Titel und der eigentlichen Ausgabe.
Es sollte darauf geachtet werden das nicht zu viele Zeilen ausgeben werden da immer die gesamte Datei angezeigt wird. Die Zeilen werden rechts einfach abgeschnitten und nicht umgebrochen!


TVScraper & scraper2vdr
-------------------------------
Since version 0.3.0 the skin support TVScraper & scraper2vdr.
With both plugins you'll get poster, banner and actor images for recordings and epg info.
If You use scraper2vdr, which I recommend, you'll also get movie and series information.


epgd & doppelte Informationen in EPG-Text
-------------------------------
Wenn epgd + epg2vdr verwendet wird, wird der angezeigte EPG-Text über die eventsview.sql festgelegt (in der epgd.conf Option: EpgView).
Mit der default eventsview.sql ist im EPG-Text die Schauspieler, Serien- und Filminformationen mit enthalten. Da diese dann doppelt angezeigt werden würden (im EPG-Text und in den extra Bereichen über scraper2vdr) existiert im contrib-Ordner von flatPlus eine eigene "eventsview-flatplus.sql". Mit dieser wird im EPG-Text wirklich nur der EPG-Text ausgeben und keine weiteren Informationen.
Ich empfehle diese zu verwenden. Dafür einfach die Datei aus den contrib Ordner nach /etc/epgd/ kopieren und in der epgd.conf folgenden Eintrag verwenden:

EpgView = eventsview-flatplus.sql

Wenn externes EPG verwendet wird, empfehle ich

EpgView = eventsview-MV.sql


Themes und Theme spezifische Icons
-------------------------------
Der Skin ist weitestgehend über Themes anpassbar.
Die Decorations (Border, ProgressBar) sind über das Theme einstellbar. Dabei kann jeweils der Typ und
die Größe (in Pixeln) eingestellt werden. Dabei wird von dem ARGB im Theme nur B verwendet. Es muss darauf geachtet werden
das die Werte in Hex angegeben werden. Wenn man also z.B. eine Größe von 20 Pixeln angeben möchte heißt der Wert: 00000014
Siehe dazu die Beispiele.

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
Beispiel
    clrChannelProgressType = 00000008
    clrChannelProgressSize = 0000000F
