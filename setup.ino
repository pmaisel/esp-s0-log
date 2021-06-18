// ########################################################################################
// ##### Start WiFi #######################################################################
// ########################################################################################

void setupWifi()
{
  spn("");
  spn("Wifi connect ...");
  WiFi.persistent(false); // Verhindert das neuschreiben des Flash
  WiFi.setOutputPower(20.5);
  WiFi.mode(WIFI_STA);
  while (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    for (byte i=0; i < 2; i++)
    {
      spn("Check SSID: " + String(ssid[i]));
      WiFi.begin(ssid[i], password[i]);
      delay(5000);
      if (WiFi.waitForConnectResult() == WL_CONNECTED) {
        spn("WiFi connected!");
        break;
      } 
      else
      {
        if (i >= 2) i = 0;
      }
    }
  }
  WiFi.hostname(host);
  spn("Done");
  sp("IP address: ");
  spn(WiFi.localIP());
}

// ########################################################################################
// ##### Sync internal time() with NTP Pool ###############################################
// ########################################################################################

void setupNtp()
{
  write2logfile("Time synchronization ...");
  getExternalTime NtpTime = &syncProvider;
  setSyncProvider(NtpTime);
  setSyncInterval(5); //re-sync after 5s
  while (timeStatus() != 2) // 2 = timeSet
  {
     spn(String(timeStatus()));
     delay(5000);
     spn("Time sync ...");
  }
   
  setSyncInterval(21600); //re-sync after 6h
  
  if( summertime_EU(year(), month(), day(), hour(), 1) )
  {
    sommerzeit = true;
    t = now() + 3600;
  }
  else
  {
    sommerzeit = false;
    t = now();
  }

  today.year  = year(t); 
  today.month = month(t)-1;  // -1 wegen dem Array das mit Null beginnt
  today.day   = day(t)-1;
    
  write2logfile("Current Time (" + String(sommerzeit?"MESZ":"MEZ") +"): " + printTime(t));
  write2logfile("Current Date: " + printDate(t));
}

// ########################################################################################
// ##### Setup HTTP Server ################################################################
// ########################################################################################

void setupHttpServer()
{
  server.on("/", handleRoot);               // Startseite
  server.on("/V", handleJsonArgs);          // Anfrage von sonnenertrag.eu bearbeiten
  server.on("/ertrag.html", handleViewErtrag); // Tageserträge als Array ausgeben
   
  server.on("/setup.html", []() {
    if (!server.authenticate(www_username, www_password)) {
      return server.requestAuthentication();
    }
    handleSetup();
  });

  server.on("/spiffs.html", []() {
    if (!server.authenticate(www_username, www_password)) {
      return server.requestAuthentication();
    }
    handleSpiffs();
  });

  server.on("/upload", HTTP_POST, [](){}, handleFileUpload);   // SPIFFS upload
  
  server.serveStatic("/screen.css", SPIFFS, "/screen.css");
  server.serveStatic("/mobile.css", SPIFFS, "/mobile.css");
  server.serveStatic("/backup.csv", SPIFFS, "/backup.csv");
  //server.serveStatic("/config.json", SPIFFS, "/config.json");
  server.serveStatic("/log.txt", SPIFFS, "/log.txt");
  
  Serial.println("HTTP Server gestartet");
}

// ########################################################################################
// ##### Ausgabearray initialisieren ######################################################
// ########################################################################################
void setupStructAusgabe()
{
  strcpy( ausgabe[0].realName, "Tagesertrag");
  //dtostrf(0, 2, 0, ausgabe[0].printValue);
  strcpy( ausgabe[0].unit, " kWh" );
  
  strcpy( ausgabe[1].realName, "Monatsertrag");
  //dtostrf(0, 2, 0, ausgabe[1].printValue);
  strcpy( ausgabe[1].unit, " kWh" );
  
  strcpy( ausgabe[2].realName, "Jahresertrag");
  //dtostrf(0, 2, 0, ausgabe[2].printValue);
  strcpy( ausgabe[2].unit, " kWh" );
  
  strcpy( ausgabe[3].realName, "Spezifisch");
  dtostrf(0, 2, 0, ausgabe[3].printValue);
  strcpy( ausgabe[3].unit, " kWh/kWp" );
  
  strcpy( ausgabe[4].realName, "Vergütung/Jahr");
  dtostrf(0, 2, 0, ausgabe[4].printValue);
  strcpy( ausgabe[4].unit, " €" );
  
  strcpy( ausgabe[5].realName, "Maximal");
  dtostrf(0, 2, 0, ausgabe[5].printValue);
  strcpy( ausgabe[5].unit, " W" );
  
  strcpy( ausgabe[6].realName, "Einspeiseleistung");
  dtostrf(0, 2, 0, ausgabe[6].printValue);
  strcpy( ausgabe[6].unit, " W" );
  
  strcpy( ausgabe[7].realName, "Voltage");
  dtostrf((ESP.getVcc()/1000.0), 2, 2, ausgabe[7].printValue);
  strcpy( ausgabe[7].unit, " V" );
  
  strcpy( ausgabe[8].realName, "RSSI");
  dtostrf(WiFi.RSSI(), 2, 0, ausgabe[8].printValue);
  strcpy( ausgabe[8].unit, " dB" );

  strcpy( ausgabe[9].realName, "Uptime");
  dtostrf(0, 2, 0, ausgabe[9].printValue);
  strcpy( ausgabe[9].unit, " min" );

  strcpy( ausgabe[10].realName, "Zaehlerstand");
  dtostrf(0, 2, 0, ausgabe[10].printValue);
  strcpy( ausgabe[10].unit, " kWh" );
}

// ########################################################################################
// ##### Restore Data from EEPROM #########################################################
// ########################################################################################

void setupEeprom()
{
  // Size in byte  
  // tagesCounter long = 4 Byte * 12Monate * 31Tage = 1488 Byte
  // setup 
  // Imp/KWh int 1488 + 4 Byte => 1492 
  // Simulation 1490 + 4 Byte => 1494 

  float floatTmp;
  int intTmp;
  char ip[4][3];
  EEPROM.begin(1520); 
  
  write2logfile("Restore Data from EEPROM ...");
  for (byte j = 0; j <= anzahlMonate; j++)
  {
    for (byte i = 0; i <= anzahlTage; i++)
    {
      EEPROM.get( eeAddress(j,i), tagesCounter[j][i]);
      //write2logfile("ADDR: " + String(eeAddress(j,i)) + " Datum: " + String(i<10?"0":"") + String(i) + "." + String(j<10?"0":"") + String(j) + " Value: " + String(tagesCounter[j][i]) );

      // Erträge in den RAM laden / Erträge stehen als Wh im EEPROM
      // Tagesertrag
      if ( j == month()-1 && i == day()-1)
      { 
        ausgabe[0].value = tagesCounter[j][i];
        //write2logfile("Tagesertrag " + String(i<10?"0":"") + String(i) + "." + String(j<10?"0":"") + String(j) + ". (Value) wiederhergestellt: " + String(ausgabe[0].value));
      }
      
      // Moatsertäge
      if ( j == today.month) 
        ausgabe[1].value+= tagesCounter[j][i];

      // Jahreserträge
      ausgabe[2].value+= tagesCounter[j][i];
      delay(5);
    }
  }
      
  EEPROM.get( adresseEepromZK,floatTmp );
  if( floatTmp > 0 )
    impulseProKwh = floatTmp;
  else
    impulseProKwh = 1000;
  write2logfile("Restore Zaehlerkonstante: " + String(adresseEepromZK) + " Value: " + String(impulseProKwh) );
  
  EEPROM.get( adresseEepromCO,floatTmp );
  if( floatTmp > 0 )
    compensation = floatTmp;
  else
    compensation = 0;
  write2logfile("Restore Verguetung: " + String(adresseEepromCO) + " Value: " + String(compensation) );
      
  EEPROM.get( adresseEepromGL,floatTmp );
  if( floatTmp > 0 )
    generatorLeistung = floatTmp;
  else
    generatorLeistung = 0;
  write2logfile("Restore Generatorleistung: " + String(adresseEepromGL) + " Value: " + String(generatorLeistung) );
    
  EEPROM.get( adresseEepromOF,floatTmp );
  if( floatTmp > 0)
    offset = floatTmp;
  else
    offset = 0;
  ausgabe[10].value = offset*1000; // Wh 
  dtostrf(offset, 2, 2, ausgabe[10].printValue);
  
  write2logfile("Restore Zaehleroffset: " + String(adresseEepromOF) + " Value: " + String(offset) );

  EEPROM.get( adresseEepromSM,floatTmp );
  if( floatTmp > 0)
    interval = floatTmp;
  else
    interval = 0;
  write2logfile("Restore Simulation: " + String(adresseEepromSM) + " Value: " + String(interval) );
    
  EEPROM.get( adresseEepromFhemPort,floatTmp );
  if( floatTmp > 0 and floatTmp < 65535)
    fhemPort = floatTmp;
  else
    fhemPort = 8383;
  write2logfile("Restore FHEM Port: " + String(adresseEepromFhemPort) + " Value: " + String(fhemPort) );

  for(byte i = 0; i <= 3; i++)
  {
    fhemHost[i] = EEPROM.read(adresseEepromFhemHost+(i*2));
  }
  write2logfile("Restore FHEM Host: " + String(fhemIp()));

  #if (DEBUG == 1)
    EEPROM.get( adresseEepromSM,intTmp); 
    if( intTmp > 0 ) 
      interval = intTmp;
    else
      interval = 3600;
    spn("Restore Simulation: " + String(adresseEepromSM) + " Value: " + String(interval) );
  #endif  
    
 


  if(generatorLeistung > 0) 
    floatTmp = ausgabe[2].value/generatorLeistung/1000.0;
  dtostrf(floatTmp, 2, 1, ausgabe[3].printValue);

  dtostrf(ausgabe[0].value/1000.0, 2, 2, ausgabe[0].printValue);
  write2logfile("Tagesertrag (printValue) wiederhergestellt: " + String(ausgabe[0].printValue));
  
  if(ausgabe[1].value > 0)
    dtostrf(ausgabe[1].value/1000.0, 2, 2, ausgabe[1].printValue);
  else
    dtostrf(0, 2, 0, ausgabe[1].printValue);

  if(ausgabe[2].value > 0)
  {
    dtostrf(ausgabe[2].value/1000.0, 2, 2, ausgabe[2].printValue);
    ausgabe[3].value = ausgabe[2].value/(generatorLeistung/1.0);
    dtostrf(ausgabe[3].value, 2, 2, ausgabe[3].printValue);
  }
  else
    dtostrf(0, 2, 0, ausgabe[2].printValue);
    
  spn("Setup EEPROM Done");
}

// ########################################################################################
// ##### FS  Setup #####################################################################
// ########################################################################################

void setupFS ()
{ 
  spn("Binde Dateisystem ein. Bitte warten ...");
  if (!SPIFFS.begin())
  {
    spn("Dateisystem konnte nicht eingebunden werden!");
  }
  #if (DEBUG == 1) 
  else
  {
    String str = "";
    Dir dir = SPIFFS.openDir("/");
    while (dir.next()) 
    {
      str += dir.fileName();
      str += " / ";
      str += dir.fileSize();
      str += " Bytes";
      str += "\r\n";
    }
    sp(str);
    write2logfile("Setup SPIFFS Done");
  }
  #endif

}


