# Uhr mit Punktanzeige 

Eines Tages kam mir beim Betrachten des Matrixdisplays die Idee, dass es nicht schlecht wäre, wenn die Textänderungen animiert wären. Es wurde interessant zu versuchen. Animationsprototyp. Das Aussehen der Schrift hat mir so gut gefallen, dass ich beschlossen habe, eine Uhr darauf basierend zu machen. 

![Punktansicht] (Punktansicht-320.gif) 

## Hardware auswählen 

Ich habe mich für den ESP32-Mikrocontroller entschieden, weil er über WLAN verfügt, mit dem Sie die Uhr mit Zeitservern synchronisieren und Informationen über das aktuelle Wetter und die Vorhersage erhalten können. Der Mikrocontroller ist sehr vielseitig, leistungsstark und nicht teuer. 

![ESP 32] lolin0](https://www.az-delivery.de/cdn/shop/products/esp32-lolin-lolin32-wifi-bluetooth-dev-kit-mikrocontroller-684175.jpg?v=1679400714&width=400) 

Ich habe verschiedene Displays ausprobiert. Infolgedessen bin ich zu dem Schluss gekommen, dass, wenn Sie nachts auf die Uhr schauen möchten, die Beleuchtung minimal oder besser noch keine sein sollte. Alle Hobby-Displays, die ich durch meine Hände gegangen bin, hatten ein schwerwiegendes Problem mit der Hintergrundbeleuchtung, außer OLED. Die Belichtung wird zu einem wichtigen Kriterium, wenn die Uhr nachts gesehen werden soll, wenn außer der Uhr kein anderes Licht vorhanden ist. Leider besteht das Problem bei OLED-Displays darin, dass es schwierig ist, das richtige in Größe, Auflösung und Preis zu finden. Die Bildschirmauflösung sollte nicht sehr groß sein; Sie möchten nicht viel Speicherplatz auf einem virtuellen Bildschirm aufwenden. Im Gegenteil, Sie möchten größere Größen, damit Sie das Bild von weitem sehen können. Ich habe die Tatsache ausgenutzt, dass die Informationen auf dem Bildschirm auf der einen Seite in halbe Stunden und auf der anderen Seite in Minuten unterteilt werden können. Somit besteht der Uhrenbildschirm aus zwei 1,5 "OLED-Displays mit einer Auflösung von jeweils 128 x 128. 

![OLED-SSD1351](https://m.media-amazon.com/images/I/61tp4yL59+L._AC_SX679_.jpg ) 

Zusätzlich habe ich mich für einen Fotowiderstand entschieden, um das Umgebungslicht zu messen, um die Helligkeit des Bildes automatisch an die Beleuchtung anzupassen. 

Alles ist auf einem Steckbrett montiert. 

Das erste Gehäuse, auf das ich gestoßen bin, hatte eine transparente Abdeckung, in die die Uhr eingelegt ist. Gekauft bei Amazon für etwa 10 Euro. 

![Anschlussdose] (https ://m.media-amazon.com/images/I/51WUj53rYDL._AC_UF894 ,1000_QL80_.jpg ) 

## Betrieb 

### Einstellung 

Wenn Sie das Programm zum ersten Mal einschalten, erkennt es, dass keine Daten für die Verbindung mit dem Netzwerk vorhanden sind, und zeigt einen QR-Code für das Programm `ESP SoftAP Provisioning` an, das sowohl im Google Play Market als auch im App Store verfügbar ist. Nach dem Einrichten von WLAN möchte das Programm Daten über seinen Standort empfangen, um die Zeitzone und den Standort herauszufinden, um Uhrzeit und Wetter korrekt anzuzeigen. Da zunächst keine Daten vorhanden sind, müssen diese über die Weboberfläche eingegeben werden. Um die Weboberfläche einzuschalten, müssen Sie die Taste auf der Rückseite der Uhr drücken; auf dem linken Bildschirm wird ein QR-Code mit der Website-Adresse und auf dem rechten Bildschirm dieselbe Adresse in normaler symbolischer Form angezeigt. Indem Sie zur Adresse gehen und die Daten mit dem API-Schlüssel von der Wetterseite eingeben [openweathermap.org ](https://openweathermap.org/api ), sowie entweder die manuelle Eingabe des Standorts und die Auswahl der Zeitzone oder die Eingabe des API-Schlüssels [ipinfo.io ] (https://ipinfo.io ), dann wird das Programm seinen Standort automatisch bestimmen. 

Unmittelbar nach dem Senden der Daten schaltet sich das Webinterface aus und die Uhr kehrt in den Normalmodus zurück. 

### Normalbetrieb 

Nachdem die Uhr ihren Standort bestimmt und die Zeitzone entsprechend angepasst hat, fragt sie regelmäßig eine Wetterseite ab, von der sie Sonnenauf- und -Untergangszeiten für den Standort erhält. Daraus wird die Farbe der Zeitziffern berechnet. Nachts sollte die Farbe kupferfarben sein, tagsüber weiß, der Übergang von einem zum anderen ist glatt. Die Farbe von Wettersymbolen und Temperaturzahlen wird aus der Temperatur berechnet. Wenn es kalt ist, wird die Farbe blau; Wenn es mäßig warm ist, ist es grün; heißes Wetter wird rot angezeigt. Formeln zur Umwandlung von Temperatur in Farbe wurden unabhängig voneinander erfunden, aus irgendeinem Grund gibt es nirgendwo etwas Vorgefertigtes. Auf dem linken Bildschirm wird im Hintergrund das Symbol für das aktuelle Wetter angezeigt, rechts die Vorhersage für die nächsten drei Stunden. Praktisch am Morgen, um zu wissen, worauf Sie sich verlassen müssen, bevor Sie zur Arbeit gehen. 

## Programm 

### Plattform 

Das Programm ist mit dem proprietären ESP-IDF-Framework von Espresiff in reinem C geschrieben. 

Das [mjson](https://github.com/cesanta/mjson ) Bibliothek wurde auch verwendet, um akzeptierte Antworten von API-Diensten zu analysieren. Ich musste eine Bibliothek eines Drittanbieters verwenden, die nicht einmal auf den ESP32 portiert war, da die mit ESP-IDF gelieferte cJSON-Komponente beim Parsen einer 16-Kilobyte-Wettervorhersageantwort mehr Speicher benötigen konnte, als im Heap frei war. 

Um mit Displays zu arbeiten, habe ich meine eigene Komponente geschrieben. Beim Schreiben lag der Schwerpunkt auf Vielseitigkeit, Effizienz, Leichtigkeit und Erweiterbarkeit. Das Hinzufügen eines virtuellen Bildschirms, dessen Bild gleichzeitig auf zwei physischen Bildschirmen angezeigt wird, dauerte mehrere Dutzend Zeilen. Die Bildrate beträgt unter Berücksichtigung, dass die Informationen zuerst auf dem virtuellen Bildschirm gezeichnet werden müssen, ungefähr 58,5 Bilder / Sekunde. Die Komponente unterstützt auch Schriftarten, die in verschiedenen Formaten dargestellt werden, z. B. BITSTREAM, wie in Bibliotheken von Adafruit, sowie Zeichen, die durch die Koordinaten von Punkten in der Matrix spezifiziert werden. Unicode-Zeichenkodierung wird unterstützt, es können nur die erforderlichen Zeichensätze in das Programm geladen werden, die Anzahl der Sätze ist eindeutig nicht begrenzt. Die Komponente wird ständig nach meinen Bedürfnissen weiterentwickelt. 
Treiber für verschiedene Displays auf verschiedenen möglichen Bussen, mit unterschiedlichen Farbtiefen und Speicherorganisation, virtuelle Bildschirme und deren Kombinationen werden unterstützt.

## Schriftart

### Hintergrund

Wir können sagen, dass die Schriftarten in diesem Projekt das Hauptmerkmal sind. Ich habe schon Projekte gemacht, bei denen Schriftarten mit kubischen Bezier-Kurven beschrieben wurden. Beschreibung mit Kurven ermöglicht es Ihnen, Symbole mit einer kleinen Anzahl von Ankerpunkten zu beschreiben, aber gleichzeitig einfach auf jede Größe zu skalieren. Ein Beispiel für ein solches Projekt ist [Wetteruhr](https://github.com/jef-sure/ili9341_dgx ). Demo-Video dieses Projekts: [Bezier Curves Clock](https://youtu.be/7H-2-X1M7PA?si=46Ek5eJoc2KWcRSB ).

### Gepunktete Schrift

Wie der Name schon sagt, besteht die gepunktete Schrift aus Punkten. Die Punkte stellen Matrizen dar, deren Größe vom gewählten Maßstab abhängt. Matrizen der Größen von 1 bis 5 werden von Hand ausgewählt, der Rest entsteht durch automatisch gezeichnete Kreise mit abnehmender Helligkeit von der Mitte zum Rand. Die Matrizen sind monochrom. Bei der Ausgabe können Sie eine Farbkarte angeben. Dank dieses Ansatzes wird die Schrift in jedem Maßstab gut aussehen.

### Morph

Um den Übergang von einem Charakter zum anderen zu animieren, werden mehrere Schritte ausgeführt. Es werden zwei Koordinatenarrays von Punkten jedes Symbols erstellt: von denen aus die Transformation erfolgt und in die die Transformation erfolgt. Dann wird die Anzahl der Elemente in diesen Arrays ausgeglichen, indem die fehlenden Elemente durch Multiplizieren des letzten Elements in dem entsprechenden Array vervollständigt werden. Die Funktion, die die Transformation durchführt, berechnet die Verschiebung jedes Punktes entlang der Linie von alten zu neuen Koordinaten in Abhängigkeit vom Parameter `t ∈ [0, 1]`, wobei `t = 0` die Koordinaten des ursprünglichen Symbols und `t = 1` - die Koordinaten des endgültigen Symbols bedeutet. Der Parameter 't' wird zeitlich so berechnet, dass das gesamte Intervall von 0 bis 1 in einer halben Sekunde vergeht. Der Startpunkt wird eine halbe Sekunde vor der nächsten Sekunde ausgewählt.

## Ergebnis

### Interne Organisation

Alle Komponenten sind auf einem Steckbrett installiert. Das Entwicklungsboard NodeMCU-P32S ist im Bett installiert. Über der Platine befinden sich zwei Displays. Die Stromversorgung erfolgt über einen externen USB-C-Anschluss. Zusätzlich installierter Taster zum Aktivieren des Konfigurationsmodus über die Weboberfläche und ein Fotowiderstand zum Anpassen der Helligkeit an die Umgebungsbeleuchtung.

### Galerie

![pcb-innen.png](Leiterplatte-innen.png)
![pcb-Seite.png](Leiterplattenseite.png)
![pcb-Anzeigen.png](Leiterplatten-Anzeigen.png)
![uhrengehäuse.png](Uhrgehäuse.png)
![uhr an der Wand.png](Uhr an der Wand.png)