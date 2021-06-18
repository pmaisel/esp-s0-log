// ########################################################################################
// ##### Setup HTTP Server ################################################################
// ########################################################################################

String contentOutputHeader()
{
  String content = "<!DOCTYPE HTML><html>";
  content+= "<head>\n";
  content+= "<meta http-equiv='content-type' content='text/html; charset=utf-8'/>\n";
  content+= "<link rel='stylesheet' href='https://www.w3schools.com/w3css/4/w3.css'/>\n";
  content+= "<link rel='stylesheet' type='text/css' media='screen' href='screen.css'/>\n";
  content+= "<link rel='stylesheet' type='text/css' media='only screen and (max-device-width: 480px)' href='mobile.css' />\n";
  content+= "<title>" + String(host) + "</title>\n";
  content+= "</head>\n";

  return content;
  content="";
}

String navigation()
{
  String content = "";
  content+= "<div><a href='/' class='w3-button w3-orange'>Home</a></div>\n";
  content+= "<div><a href='/ertrag.html' class='w3-button w3-orange'>Ertrag</a></div>\n";
  content+= "<div><a href='/setup.html' class='w3-button w3-orange'>Setup</a></div>\n";
  content+= "<div><a href='/spiffs.html' class='w3-button w3-orange'>SPIFFS</a></div>\n";
  content+= "<div><a href='/update' class='w3-button w3-orange'>Update</a></div>\n";
  //content+= "<a href='/V?f=j&u=Wh&m=" + String(month()) + "' class='w3-button w3-orange'>Json</a>\n";
  
  return content;
}

void contentOutput(String content)
{
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", content);
  content = "";
}

// ########################################################################################
// ##### Web Root #########################################################################
// ########################################################################################

void handleRoot()   
{ 
  char queryRealName[7][20] = {"Tagesertrag","Jahresertrag","Spezifisch","Einspeiseleistung","Maximal","Voltage","RSSI"};
    
  String divRow = "<div class='row'>";
  String divlP =  "<div class='lP'>";
  String divVal = "<div class='value'>";
  
  String content = contentOutputHeader();
  content+= "<body>";
  content+= "<div class='w3-container'>";
  content+= "<h1>ESPLog S0 Datenlogger</h1>";    
  content+= "</div>";
  content+= "<div class='navi'>";
  content+= navigation();
  content+= "</div>";
  content+= "<div class='w3-container'>";
  content+= "<div class='rahmen'>";

  content+= "<div class='borderNamePos'><span class='borderName'>Aktuell</span></div>\n";
  content+= "<div class='row'><div class='lP'>Datum:</div><div class='value'>" + printDate(t) + "</div><div class='unit'>&nbsp;</div></div>\n";
  content+= "<div class='row'><div class='lP'>Uhrzeit:</div><div class='value'>" + printTime(t) + "</div><div class='unit'>&nbsp;</div></div>\n";
  content+= "<div class='row'><div class='lP'>Sommerzeit:</div><div class='value'>" + (sommerzeit ? String("ja"):String("nein")) + "</div><div class='unit'>&nbsp;</div></div>\n";
  
  for( int i = 0; i < lenStore; i++)
  {
    for( int j = 0; j < (sizeof(queryRealName)/sizeof(queryRealName[0])); j++ )
    {
      if( strncmp(ausgabe[i].realName,queryRealName[j],20) == 0 )
      {
        content+= "<div class='row'><div class='lP'>" + String(ausgabe[i].realName) + ":</div><div class='value'>" + String(ausgabe[i].printValue)  + "</div><div class='unit'>" + String(ausgabe[i].unit) + "</div></div>";
        break;
      }
    }
  }
  
  content+= "<div class='row'><div class='lP'>Z&auml;hlerstand:</div><div class='value'>" + String(ausgabe[10].printValue) + "</div><div class='unit'> kWh</div></div>\n";
  content+= "<div class='row'><div class='lP'>Laufzeit:</div><div class='value'>" + String(ausgabe[9].printValue) + "</div><div class='unit'> min</div></div>\n";
  content+= "<div class='row'><div class='lP'>Free RAM:</div><div class='value'>" + String(ESP.getFreeHeap()) + "</div><div class='unit'> Byte</div></div>\n";
  
  content+= "</div>";

  content+= "</div>";
  content+= "</body></html>";
  
  contentOutput(content);
  content = "";
}

// ########################################################################################
// ##### Web Setup ########################################################################
// ########################################################################################

void handleSetup() 
{   
  float f = 0.00f;
  int intTmp;
  char charVal[15] = "0";
  
  String W3C = "<div class='w3-container'>";
  String divBox = "<div class='box'>";
  String divlP =  "<div class='lP'>";
  
  String content = "";
  
  for (int i = 0; i < server.args(); i++) 
  {
    if ( server.arg(i) != "")
    {
      // ##### Generatotrleistung setzen
      if(server.argName(i) == "generator" and server.arg(server.argName(i)).toInt() != generatorLeistung )
      {
        generatorLeistung = server.arg(server.argName(i)).toInt();
        write2logfile("Neue Generatorleistung setzen: " + String(generatorLeistung) );
        putEepromValue(adresseEepromGL, generatorLeistung);
      }
      
      // ##### Zählerkonstatante setzen
      if(server.argName(i) == "couterconst" and server.arg(server.argName(i)).toInt() != impulseProKwh )
      {
        impulseProKwh = server.arg(server.argName(i)).toInt();
        write2logfile("Neue Zaehlerkonstante setzen: " + String(impulseProKwh) );
        putEepromValue(adresseEepromZK, impulseProKwh);
      }

      // ##### Neuen Zähleroffset setzen
      if(server.argName(i) == "offset" and server.arg(server.argName(i)).toFloat() != offset )
      {
        offset = server.arg(server.argName(i)).toFloat();
        write2logfile("Zaehleroffset setzen: " + String(offset) );
        putEepromValue(adresseEepromOF, offset);
        f = ausgabe[10].value;
        ausgabe[10].value = f + (offset*1000);
        dtostrf(ausgabe[10].value, 2, 2, ausgabe[10].printValue);
      }
      
      // debug *****************************
      // ##### Interval für Simulation setzen 
      #if (DEBUG == 1) 
      if(server.argName(i) == "simulation" and server.arg(server.argName(i)).toFloat() != interval )
      {
        interval = server.arg(server.argName(i)).toFloat();
        write2logfile("Timer Simulation setzen: " + String(interval) );
        putEepromValue(adresseEepromSM,interval);
        os_timer_arm(&Timer1, interval, true);
      }
      #endif

      // ##### Vergütung setzen
      if(server.argName(i) == "compensation" and server.arg(server.argName(i)).toFloat() != compensation )
      {
        compensation = server.arg(server.argName(i)).toFloat();
        write2logfile("Neue Verguetung setzen: " + String(compensation) );
        putEepromValue(adresseEepromCO,compensation);
      }

      // ##### Tageswerte aus dem EEPROM im RAM komplett sichern
      if(server.argName(i) == "backup")
      {
        write2logfile("Tageswerte gesichert");
        backup2file ();
      }
      
      // ##### Tageswerte im EEPROM komplett löschen
      if(server.argName(i) == "clrrom")
      {
        write2logfile("Tageswerte im Flash geloescht");
        deleteEEPROM ();
        deleteRAM ();
      }

      // ##### Controller IP schreiben
      if(server.argName(i) == "ip" )
      {
        //char ipStr[20];
        //sprintf(ipStr, "%u.%u.%u.%u", fhemHost[0], fhemHost[1], fhemHost[2], fhemHost[3]);

        if(server.arg(server.argName(i)) != fhemIp())
        {
          char *tokens;
          int i = 0;
          char buf[20];
          server.arg(server.argName(i)).toCharArray(buf,20);
          tokens = strtok(buf, ".");
      
          while (tokens != NULL) {
              fhemHost[i] = atoi(tokens);
              EEPROM.write(adresseEepromFhemHost+(i*2),fhemHost[i]);
              delay(2);
              tokens = strtok(NULL, ".");
              i++;
          }
          delay(200);
          EEPROM.commit();
          
          write2logfile("Neue Controller IP setzen: " + String(fhemIp()) );
          spn("Stored Controller IP: " + String(fhemIp()));
        }
      }
      
      // ##### Controller Port schreiben
      if(server.argName(i) == "port" and server.arg(server.argName(i)).toInt() != fhemPort )
      {
        fhemPort = server.arg(server.argName(i)).toInt();
        write2logfile("Controller Port schreiben: " + String(server.arg(server.argName(i)).toInt()) );
        putEepromValue(adresseEepromFhemPort,fhemPort);
      }

      // ##### Restart ausführen
      if(server.argName(i) == "restart")
      {
        write2logfile("Restart durch Benutzer");
        ESP.restart();
      }
    } 
  } // for
  
  
  content+= contentOutputHeader();
  
  content+= "<body>";
  content+= W3C;
  content+= "<h1>Setup</h1>";                     // Antwort für Webbrowser vorbereiten
  content+= "</div>";
  content+= "<div class='navi'>";
  content+= navigation();
  content+= "</div>";

  // ------------------ //
  
  content+= W3C;
  content+= "</div>";
  content+= "<form action='/setup.html' name='vorgaben' method='POST'>";
  
  content+= W3C;
  content+= "<div class='rahmen'>";
  content+= "<div class='borderNamePos'><span class='borderName'>Allgemein</span></div>";

  content+= "<div class='box'>";
  content+= "<div class='lP'><label for='generator'>Generatorleistung</label></div>";
  content+= "<input id='generator' name='generator' type='text' value='" + String(generatorLeistung) + "'> Wp";
  content+= "</div>";

  content+= "<div class='box'>";
  content+= "<div class='lP'><label for='couterconst'>Z&auml;hlerkonstante</label></div>";
  content+= "<input id='couterconst' name='couterconst' type='text' value='" + String(impulseProKwh) + "'> Imp/KWh";
  content+= "</div>";

  content+= "<div class='box'>";
  content+= "<div class='lP'><label for='compensation'>Verg&uuml;tung</label></div>";
  content+= "<input id='compensation' name='compensation' type='text' value='" + String(compensation) + "'> cent/kWh";
  content+= "</div>";

  content+= "<div class='box'>";
  content+= "<div class='lP'><label for='offset'>Z&auml;hleroffset</label></div>";
  content+= "<input id='offset' name='offset' type='text' value='" + String(offset) + "'> kWh";
  content+= "</div>";
  
#if (DEBUG == 1) 
  content+= "<div class='box'>";
  content+= "<div class='lP'><label for='simulation'>Simulation</label></div>";
  content+= "<input id='simulation' name='simulation' type='text' value='" + String(interval) + "'> ms";
  content+= "</div>";
#endif
  content+= "<div class='box'>";
  content+= "<div class='lP'><label for='save'>Speichern</label></div>";
  content+= "<input class='w3-button w3-orange' name='save' type='submit' value='OK'>";
  content+= "</div>";
  
  content+= "</div>";
  
  content+= "</form>";

  // ------------------ //
  
  content+= "<div class='rahmen'>";
    content+= "<div class='borderNamePos'>";
      content+= "<span class='borderName'>Ert&auml;ge</span>";
    content+= "</div>";
  
    content+= "<div class='box'>";
      content+= "<div class='lP' style='margin-top: 8px;'><label for='backup'>Ertragsdaten</label></div>";
        content+= "<input class='w3-button w3-orange btbackup' name='backup' type='submit' value='sichern'>";
        content+= "<input class='w3-button w3-orange btdelete' name='clrrom' type='submit' value='l&ouml;schen'>\n";
    content+= "</div>";
  content+= "</div>";

  // ------------------ //

  content+= "<form action='/setup.html' name='controller' method='POST'>";
     content+= "<div class='rahmen'>";
      content+= "<div class='borderNamePos'><span class='borderName'>Controller</span></div>";
    
      content+= "<div class='box'>";
      content+= divlP + "<label for='ip'>IP Adresse</label></div>";
      content+= "<input id='fhemHost' name='ip' type='text' value='" + String(fhemIp()) + "'>";
      content+= "</div>";
      
      content+= "<div class='box'>";
      content+= divlP + "<label for='port'>Port</label></div>";
      content+= "<input id='fhemPort' name='port' type='text' value='" + String(fhemPort) + "'>";
      content+= "</div>";
    
      content+= "<div class='box'>";
      content+= divlP + "<label for='save'>Speichern</label></div>";
      content+= "<input class='w3-button w3-orange' name='save' type='submit' value='OK'>";
      content+= "</div>";
      
    content+= "</div>";
   content+= "</form>";

  // ------------------ //
  
  content+= "<div class='rahmen'>";

  content+= "<div class='borderNamePos'><span class='borderName'>Netzwerk</span></div>";
  content+= "<div class='row'>" + divlP + "IP-Adresse:</div>" + String(WiFi.localIP().toString()) + "</div>";
  content+= "<div class='row'>" + divlP + "Subnet:</div>" + String(WiFi.subnetMask().toString()) + "</div>";
  content+= "<div class='row'>" + divlP + "Gateway:</div>" + String(WiFi.gatewayIP().toString()) + "</div>";
  content+= "</div>";

  // ------------------ //
  
  content+= "<div class='rahmen'>";
    content+= "<div class='borderNamePos'>";
      content+= "<span class='borderName'>System</span>";
    content+= "</div>";
  
    content+= "<div class='box'>";
      content+= "<div class='lP' style='margin-top: 8px;'><label for='backup'>Neustart</label></div>";
        content+= "<input class='w3-button w3-orange btrestart' name='restart' type='submit' value='Restart'>";
    content+= "</div>";
  content+= "</div>";
  
  //content+= "</form>";
  content+= "<div>";
  
  // ##### Debug
  /*
  content+= "<div class='w3-container'>\n";
  content+= "Number of args received:";
  content+= server.args();            //Get number of parameters
  content+= "<br>\n";                     //Add a new line
  for (int i = 0; i < server.args(); i++) 
  {
    if ( server.arg(i) != "")
    {
      content+= "Arg " + (String)i + ": –> ";  //Include the current iteration value
      content+= server.argName(i) + ": " + server.arg(i);     //Get the name of the parameter and Value
      content+= "<br>";
    }
  }
  content+= "</div>\n";
  // End Debug

  */
  contentOutput(content);
  content = "";
}

// ########################################################################################
// ##### SPIFFS ###########################################################################
// ########################################################################################

void handleSpiffs ()
{ 
  FSInfo fs_info;
  SPIFFS.info(fs_info);

  if (server.hasArg("Delete")) {
    
    Serial.println("Lösche Datei: " + server.arg("file"));
    
    Dir dir = SPIFFS.openDir("/");
    while (dir.next())    {
      String path = dir.fileName();
      path.replace(" ", "%20"); path.replace("ä", "%C3%A4"); path.replace("Ä", "%C3%84"); path.replace("ö", "%C3%B6"); path.replace("Ö", "%C3%96");
      path.replace("ü", "%C3%BC"); path.replace("Ü", "%C3%9C"); path.replace("ß", "%C3%9F"); path.replace("€", "%E2%82%AC");
      if (server.arg("file") != path )
        continue;
      SPIFFS.remove(dir.fileName());
      write2logfile("Datei " + String(dir.fileName()) + " geloescht" );
      String header = "HTTP/1.1 303 OK\r\nLocation:";
      header += server.uri();
      header += "\r\nCache-Control: no-cache\r\n\r\n";
      server.sendContent(header);
      return;
    }
  }

  String content = contentOutputHeader();

  content+= "<body>";
  content+= "<div class='w3-container'>";
  content+= "<h1>SPIFFS</h1>";                    
  content+= "</div>";
  content+= "<div class='navi'>";
  content+= navigation();
  content+= "</div>";
  content+= "<div class='w3-container'>";
  content+= "</div>";
  
  content+= "<div class='w3-container'>";

    content+= "<div class='rahmen'>";
      content+= "<div class='borderNamePos'>";
        content+= "<span class='borderName'>Inhalt Dateisystem</span>";
          content+= "<p>";
  
          Dir dir = SPIFFS.openDir("/");
          while (dir.next()) {
            content+= "<div class='filelist'><div class='filename'>";
            content+= "<a href ='";
            content+= dir.fileName();
            content+= "?download=' target='blanc'>";
            //content+= "SPIFFS";
            content+= dir.fileName();
            content+= "</a></div> ";
            content+= "<div class='filesize'>";
            content+= formatBytes(dir.fileSize()).c_str();
            //content+= " Bytes";
            content+= "</div>";
        
            content+= "<form action='/spiffs.html' method='POST'>";
            content+= "<div class='trash'>";
            content+= "<input type='submit' name='Delete' value='X' class='btdelete' title='Datei " + dir.fileName() + " l&ouml;schen!'>";
            content+= "<input type='hidden' name='file' value='" + dir.fileName() + "'>";
            content+= "</div>";
            content+= "</form>";
            
            content+= "</div>\n";
          }
          content+= "</p>";

        content+= "<div class='filelist'>";
          content+= "<div class='filename'><label for='fs_size'>Gr&ouml;&szlig;e Dateisystem</label></div>";
          content+= formatBytes(fs_info.totalBytes).c_str();
        content+= "</div>";
        content+= "<div class='filelist'>";
          content+= "<div class='filename'><label for='free_size'>Davon Verwendet</label></div>";
          content+= formatBytes(fs_info.usedBytes).c_str();
        content+= "</div>";
  
      content+= "</div>";
  //content+= "</div>";
  
  // ---------------------------------- //
  
  //content+= "<div class='rahmen'>";
  //content+= "<div class='borderNamePos'>";
    //content+= "<span class='borderName'>Dateisystem</span>";
    
    /*
    content+= "<div class='box'>";
      content+= "<form action='/spiffs.html' method='POST'>Datei zum l&ouml;schen hier per Drag-and-Drop einf&uuml;gen<br>";
      content+= "<div class='filelist'>";
        content+= "<div class='lP'><input type='text' name='Delete' placeholder='Datei hier einf&uuml;gen' required class='delete'></div>";
        content+= "<input type='submit' class='button' name='SUBMIT' value='L&ouml;schen'>";
      content+= "</div>";
      content+= "</form>";
    content+= "</div>";
    */
    content+= "<div>&nbsp;</div>";
    content+= "<div  class='box'>";
      content+= "<form method='POST' action='/upload' enctype='multipart/form-data' style='height:35px;'>";
        content+= "<div class='filelist'>";
          content+= "<div class='lP'><input type='file' name='upload' class='upload' required></div>";
          content+= "<input type='submit' value='Speichern' class='button'>";
        content+= "</div>";
      content+= "</form>";
    content+= "</div>"; // class box
    
  content+= "</div>";
  
  content+= "</div></div>";

  contentOutput(content);
}

// ########################################################################################
// ##### Web Ausgabe JSON für sonnenertrag.eu #############################################
// ##### Dieses Projekt ist eingestellt ###################################################
// ########################################################################################

void handleJsonArgs() 
{   
  write2logfile("Web Ausgabe JSON fuer sonnenertrag.eu");
  
  byte angefragterMonat;
  String content = contentOutputHeader();
  for (int i = 0; i < server.args(); i++) 
  {
    if(server.argName(i) = "m") angefragterMonat = server.arg(i).toInt();
  } 

  if (angefragterMonat >= 1 && angefragterMonat <= 12)
  {
    content += "[" ;
    for (byte i = 0; i <= anzahlTage; i++)
    {
      content += String(tagesCounter[angefragterMonat-1][i]); // angefragten Monat ausgeben
      if(i < anzahlTage) content += ",";
    }
    content += "]" ;
  }
  contentOutput(content);
}

// ########################################################################################
// ##### Spezifizierte Rückgabe Test ######################################################
// ########################################################################################
/*
void handleSpecificArg() 
{ 
  String content = "";
  content += contentOutputHeader();
  
  if (server.arg("Temperature")== "")
  {     //Parameter not found
    content = "Temperature Argument not found";
  }
  else
  {     //Parameter found
    content = "Temperature Argument = ";
    content += server.arg("Temperature");     //Gets the value of the query parameter
  }
  
  contentOutput(content);
}
*/
// ########################################################################################
// ##### Web Ausgabe gesammtes Array ######################################################
// ########################################################################################

void handleViewErtrag()               // Anfrage vom Webbrowser
{
  String content;
  char buf[15]; // flot to string convert buffer
  byte tmp, j, m;
  char *monat[] {"Januar","Februar","M&auml;rz","April","Mai","Juni","Juli","August","September","Oktober","November","Dezember"};
  
  char tagesertrag[10];
  
  content+= contentOutputHeader();
  content+= "<body>";
  content+= "<div class='w3-container'>";
  content+= "<h1>Ertrag</h1>";                     // Antwort für Webbrowser vorbereiten
  content+= "</div>";
  content+= "<div class='navi'>";
  content+= navigation();
  content+= "</div>";
  
  content+= "<div class='w3-container'>";
  content+= "<div class='rahmen'>";

  content+= "<div class='borderNamePos'><span class='borderName'>Tagesdaten in kWh</span></div>";

  if(server.arg("monat") == "") 
    j = today.month;
  else
  {
    m = server.arg("monat").toInt();
    if(m == 0) 
      j = 11;
    else if(m > 12)
      j = 0;
    else
      j = m-1;
  }

  content+= "<div class='row' style='line-height: 2.5;'><div class='back'><a href='/ertrag.html?monat=" + String(j) + "'>zur&uuml;ck</a></div><div class='monat'>" + monat[j] + "</div><div class='next'><div class='next'><a href='/ertrag.html?monat=" + String(j+2) + "'>vor</a></div></div>" ;
  
  content+= "<div>";
      
  content+= "<div id='wk'>\n";
  
  // Anzahl der Tage im Monat berechnen
  if (j == 1)
  { // Februar
    if (((year()%4==0 && year()%100!=0) || year()%400==0)) tmp = 28; else tmp = 27;
  }
  else
  {
    if (j <= 6)
    { // Januar - Juli
     if (j % 2 == 0) tmp = 30; else tmp = 29;
    }
    else
    { // August - Dezember
      if (j % 2 == 0) tmp = 29; else tmp = 30;
    }
  }

  //datatable+= "['Tag','Ertrag']";
  for (byte i = 0; i <= tmp; i++)
  {
    dtostrf((tagesCounter[j][i]/1000.0), 1, 1, buf);
      
    if (j == today.month && i == today.day)
      content += "<div id='tb' class='tabBlink'>" + String(buf) + "</div>"; // Tagescounter für den aktuellen Tag ausgeben
    else
    {
      content += "<div id='tb'>" + String(buf) + "</div>"; // Tagescounter ausgeben
    }
    if((i+1) % 7 == 0) content+= "</div>\n<div id='wk'>";
  }
  content += "</div>" ;
  
  content+= "</div></div>"; // <div class='borderNamePos'></div class='rahmen'>  
  content+= graph();
  content+= "</div>";

  contentOutput(content);
}


// ########################################################################################
// ##### File Upload  #####################################################################
// ########################################################################################

void handleFileUpload() {
  if (server.uri() != "/upload") return;
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    String filename = upload.filename;
    //sp("handleFileUpload Name: "); spn(filename);
    if (filename.length() > 30) {
      int x = filename.length() - 30;
      filename = filename.substring(x, 30 + x);
    }
    if (!filename.startsWith("/")) filename = "/" + filename;
    //sp("handleFileUpload Name: "); spn(filename);
    fsUploadFile = SPIFFS.open(filename, "w");
    filename = String(); 
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    //sp("handleFileUpload Data: "); spn(upload.currentSize);
    if (fsUploadFile)
      fsUploadFile.write(upload.buf, upload.currentSize);
  } else if (upload.status == UPLOAD_FILE_END) {
    String filename = upload.filename;
    write2logfile("Neue Datei gespeichert: " + String(filename) );
    //server.serveStatic("/" + String(filename), SPIFFS, "/" + String(filename) );
    if (fsUploadFile)
      fsUploadFile.close();
    yield();
    //sp("handleFileUpload Size: "); spn(upload.totalSize);
    handleSpiffs();
  }
}
