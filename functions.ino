// ########################################################################################
// ##### Fünf Minuten Timer ###############################################################
void FuenfMinutenTimer(byte nowMonth, byte nowDay)
{
  float tmp;
  
  if ( impulseProKwh > 0)
  {
    tmp = counter*1000/impulseProKwh; // Wh

    tagesCounter[nowMonth][nowDay] += tmp; // Wh in das Array schreiben
  
    ausgabe[0].value+= tmp;
    dtostrf((ausgabe[0].value/1000), 2, 2, ausgabe[0].printValue);
  
    ausgabe[1].value+= tmp;
    dtostrf((ausgabe[1].value/1000), 2, 2, ausgabe[1].printValue);
  
    ausgabe[2].value+= tmp;
    dtostrf((ausgabe[2].value/1000), 2, 2, ausgabe[2].printValue);
  
    ausgabe[3].value = ausgabe[2].value/generatorLeistung/1.0;
    dtostrf(ausgabe[3].value, 2, 2, ausgabe[3].printValue);

    ausgabe[10].value+= tmp;
    dtostrf((ausgabe[10].value/1000), 2, 2, ausgabe[10].printValue);
  }
  
  // Momentanleistung berechnen
  // P = UF * (n/t) = n/impulseProKwh *(n/(t/3600)) kW für (t in s); n = Anzahl Impulse
  ausgabe[6].value = counter * 3600.0/300.0;
  dtostrf(ausgabe[6].value, 2, 0, ausgabe[6].printValue);

  fuenfMinutenZaehler[(hour()*12 + minute()/5)] = ausgabe[6].value;

  // Maximalwert updaten
  if(ausgabe[6].value > ausgabe[5].value)
  {
    ausgabe[5].value = ausgabe[6].value;
    dtostrf(ausgabe[5].value, 2, 0, ausgabe[5].printValue);
  }
  
  counter = 0;           // Zähler zurücksetzen
  putEeprom(today.month, today.day); // Daten im EEPROM sichern  
}

// ########################################################################################
// ##### Erzeugt das Blinken der LED ######################################################
// ########################################################################################

void flashLed(bool onoff)
{
  //int state = digitalRead(BUILTIN_LED); // get the current state of BUILTIN_LED
  //digitalWrite(BUILTIN_LED, !state);
  if (onoff)
  {
    digitalWrite(LED_PIN1, HIGH);
    //digitalWrite(BUILTIN_LED, LOW);
    previousMillis = millis();
  }
  else
  {
    if (millis() - previousMillis >= 30) 
    {
      digitalWrite(LED_PIN1, LOW);
      //digitalWrite(BUILTIN_LED, HIGH);
    }
  }
}

// ########################################################################################
// ##### Schreibt 16bit Ertragsdaten in den Flash #########################################
// ########################################################################################

void putEeprom(byte nowMonth, byte nowDay)
{
  unsigned int tmp;
  EEPROM.get( eeAddress(nowMonth,nowDay), tmp);
  if (tmp != tagesCounter[nowMonth][nowDay])
  {
    EEPROM.put( eeAddress(nowMonth,nowDay), tagesCounter[nowMonth][nowDay]);
    delay(200);
    EEPROM.commit();
  }
}

// ########################################################################################
// ##### Speichert einzelnen 16bit Wert in den Flash ######################################
// ########################################################################################

int putEepromValue (int address, float value)
{
  float f;
  EEPROM.put(address,value);
  delay(200);
  EEPROM.commit();
  delay(200);
  EEPROM.get(address,f);
  sp("Gespeichert in Adresse: ");
  sp(address);
  sp(" value: ");
  spn(f);
}

// ########################################################################################
// ##### Berechnet die Flash Speicherstelle nach Tag und Monat ############################
// ########################################################################################

int eeAddress(byte j, byte i)
{ // j - month, i = day
    int result = ((i+1)+((j*31))-1)*4;
    return result;
}

// ########################################################################################
// ##### Formatiert Byte in Byte/KB/MB ####################################################
// ########################################################################################

String formatBytes(size_t bytes) {
  //int runtime = millis();
  if (bytes < 1024) {
    return String(bytes) + " Byte";
  } else if (bytes < (1024 * 1024)) {
    return String(bytes / 1024.0) + " KB";
  } else if (bytes < (1024 * 1024 * 1024)) {
    return String(bytes / 1024.0 / 1024.0) + " MB";
  }
  //spn("Call Funnction: formatBytes " + String(millis() - runtime));
}

// ########################################################################################
// ##### Schreibt Date in das Logfile #####################################################
// ########################################################################################

void write2logfile (String logtext)
{
  File logbuch = SPIFFS.open("/log.txt", "a+");
  if (!logbuch) 
    spn("Logbuchdatei Datei konnte nicht geöffnet werden!");

  logbuch.println(printDate(t) + " " + printTime(t) + " " + logtext);
  logbuch.flush();
  logbuch.close();
}

// ########################################################################################
// ##### Erzeugt ein Backupfile der Jahresdaten ###########################################
// ########################################################################################

void backup2file () 
{
  //int runtime = millis();
  unsigned int tmp;

  spn("Sichere Jahresdaten in Datei ...");
  File backup = SPIFFS.open("/backup.csv", "w+");
  if (!backup) 
    spn("Datei konnte nicht geöffnet werden!");
  
  spn("schreibe SPIFFS Datei");
  for (byte j = 0; j <= anzahlMonate; j++)
  { 
    backup.print(String(j+1) + ",");
    for (byte i = 0; i <= anzahlTage; i++)
    {
      EEPROM.get( eeAddress(j,i), tmp);
      backup.print(tmp);
      if(i < anzahlTage) backup.print(","); 
    }
    backup.println("");
  }
  backup.flush();
  backup.close();
  spn("Jahresdaten gesichert!");
  //spn("Call Funnction: backup2file " + String(millis() - runtime));
}

// ########################################################################################
// ##### Tageswerte im Flash komplett löschen #############################################
// ########################################################################################

void deleteEEPROM ()
{
  //int runtime = millis();
  spn("Lösche EEPROM ...");
  for (int j = 0; j < sizeof(tagesCounter); j++)
  {
  if ( j % 4 == 0) spn("Lösche Adresse " + String(j));
    EEPROM.put(j,0); // Clear
  }
  delay(200);
  spn("EEPROM übergeben!");
  EEPROM.commit();
  EEPROM.end();
  //spn("Call Funnction: deleteEEPROM " + String(millis() - runtime));
  spn("EEPROM gelöscht");
}

// ########################################################################################
// ##### Tageswerte im RAM komplett löschen ###############################################
// ########################################################################################

void deleteRAM ()
{
  memset(tagesCounter,0,sizeof(tagesCounter)); // Tagescounter löschen
  ausgabe[0].value = 0;
  dtostrf(ausgabe[0].value, 2, 2, ausgabe[0].printValue);
  ausgabe[1].value = 0;
  dtostrf(ausgabe[1].value, 2, 2, ausgabe[1].printValue);
  ausgabe[2].value = 0;
  dtostrf(ausgabe[2].value, 2, 2, ausgabe[2].printValue);
  
  counter = 0;
  spn("RAM gelöscht");
}

// ########################################################################################
// ##### Gibt die aktuelle FHEM Host IP als String zurück #################################
// ########################################################################################

String fhemIp()
{
  char s[20];
  sprintf(s, "%u.%u.%u.%u", fhemHost[0], fhemHost[1], fhemHost[2], fhemHost[3]);
  return s;
}

// ########################################################################################
// ##### Gibt die aktuelle Zeit und das Datum in deutscher Norm aus #######################
// ########################################################################################

String printTime (unsigned long t)
{
  String tmp = (String(hour(t)<10?"0":"") + String(hour(t)) + ":" + String(minute(t)<10?"0":"") + String(minute(t)) + ":" + String(second(t)<10?"0":"") + String(second(t)));
  return tmp;
}

String printDate (unsigned long t)
{
  String tmp = (String(day(t)<10?"0":"") + String(day(t)) + "." + String(month(t)<10?"0":"") + String(month(t)) + "." + String(year(t)));
  return tmp;
}

// ########################################################################################
// ##### Prüft auf Sommerzeit #############################################################
// ########################################################################################

boolean summertime_EU(int year, byte month, byte day, byte hour, byte tzHours)
// European Daylight Savings Time calculation by "jurs" for German Arduino Forum
// input parameters: "normal time" for year, month, day, hour and tzHours (0=UTC, 1=MEZ)
// return value: returns true during Daylight Saving Time, false otherwise
{
 if (month<3 || month>10) return false; // keine Sommerzeit in Jan, Feb, Nov, Dez
 if (month>3 && month<10) return true; // Sommerzeit in Apr, Mai, Jun, Jul, Aug, Sep
 if (month==3 && (hour + 24 * day)>=(1 + tzHours + 24*(31 - (5 * year /4 + 4) % 7)) || month==10 && (hour + 24 * day)<(1 + tzHours + 24*(31 - (5 * year /4 + 1) % 7)))
   return true;
 else
   return false;
}

// ########################################################################################
// ##### Tageswerte als Graph darstellen ##################################################
// ########################################################################################

String graph ()
{
  char zeit[5];
  String content = "";
  
  content+= "\n<script type='text/javascript' src='https://www.gstatic.com/charts/loader.js'></script>\n";
    content+= "<script type='text/javascript'>\n";
      content+= "google.charts.load('current', {'packages':['corechart']});\n";
      content+= "google.charts.setOnLoadCallback(drawChart);\n";

      content+= "function drawChart() {\n";
        content+= "var data = google.visualization.arrayToDataTable([\n";
        
          // Data
          content+= "['Index', 'W' ],";
          
          for (int i = 0; i < (sizeof(fuenfMinutenZaehler)/sizeof(int)); i++) {
            if (fuenfMinutenZaehler[i] > 0) 
            {
              sprintf(zeit, "%02u:%02u", i/12, (i % 12)*5);
              content+= "['" + String(zeit) + "', " + String(fuenfMinutenZaehler[i]) + "]";
              if(i < (sizeof(fuenfMinutenZaehler)/sizeof(int)-1) )
                content+= ",";
            }
          }
        
        content+= "]);\n";

        content+= "var options = { chartArea: { left: 10, right: 10 }, vAxis: { minValue:0, viewWindow: { min: 0 } }, backgroundColor: '#f9f9f9', title: 'Tagesertrag', curveType: 'function',legend: { position: 'bottom' } };\n";

        content+= "var chart = new google.visualization.AreaChart(document.getElementById('curve_chart'));\n ";
        content+= "chart.draw(data, options);\n";
      content+= "}\n";
    content+= "</script>\n";
  
  content+= "<div id='curve_chart' class='curve_chart'></div>";
  return content;
}


