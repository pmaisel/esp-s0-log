// ########################################################################################
// ##### Stelle den JSON String zuasammen #################################################
// ########################################################################################

String CreateSensorString ()
{
  String url = F("/ESPEasy");
  String jsonString = "";

  char queryRealName[9][20] = {"Tagesertrag","Jahresertrag","Spezifisch","Einspeiseleistung","Zaehlerstand","Maximal","Voltage","RSSI","Uptime"};
  //char ipStr[20];
  //sprintf(ipStr, "%u.%u.%u.%u", fhemHost[0], fhemHost[1], fhemHost[2], fhemHost[3]);

  String initialjsonString = "";
  initialjsonString+= "{\"module\":\"ESPEasy\",";
  initialjsonString+= "\"version\":\"1.02\",";
  initialjsonString+= "\"data\":";
  initialjsonString+= "{\"ESP\":{\"name\":\"EasyLog\",\"unit\":0,\"version\":9,\"build\":147,\"sleep\":0,\"ip\":\"" + String(WiFi.localIP().toString()) + "\"},";
  initialjsonString+= "\"SENSOR\":{";

  for( int i = 0; i < lenStore; i++)
  {
    for( int j = 0; j < (sizeof(queryRealName)/sizeof(queryRealName[0])); j++ )
    {
      if( strncmp(ausgabe[i].realName,queryRealName[j],20) == 0 ) // Compare ist true == 0
      {
        jsonString = initialjsonString;
        jsonString+= "\"" + String(0) + "\":";
        jsonString+= "{\"deviceName\":\"\",\"valueName\":\"" + String(ausgabe[i].realName) + "\",\"type\":6,\"value\":\"" + String(ausgabe[i].printValue) + "\"}";
        jsonString+= "}}}";
        
        FHEMHTTPsend(url, jsonString);
      }
    }
  }
  //tcpCleanup ();
}

// ########################################################################################
// ##### Sende Daten zum FHEM Controller und lese Antwort #################################
// ########################################################################################

boolean FHEMHTTPsend (String & url, String & buffer)
{
  boolean success = false;
  
  //char ipStr[20];
  //sprintf(ipStr, "%u.%u.%u.%u", fhemHost[0], fhemHost[1], fhemHost[2], fhemHost[3]);
  
  WiFiClient client;

  if(!client.connect(fhemHost, fhemPort))
  {
    spn("HTTP : connection failed");
    return false;
  }
  
  int len = buffer.length();
  client.print(String("POST ") + url + F(" HTTP/1.1\r\n") +
              F("Content-Length: ")+ len + F("\r\n") +
              F("Host: ") + fhemIp() + F("\r\n") +
              F("Connection: close\r\n\r\n")
              + buffer);

  unsigned long timer = millis() + 200;
  while (!client.available() && millis() < timer)
    yield();
  
  // Read all the lines of the reply from server and print them to Serial
  while (client.available()) {
    String line = client.readStringUntil('\n');
    String helper = line;
    //line.toCharArray(log, 80);
    
    if (line.substring(0, 15) == "HTTP/1.1 200 OK") {
      spn("HTTP : Success");
      success = true;
    }
    else if (line.substring(0, 24) == "HTTP/1.1 400 Bad Request") {
      spn("HTTP : Bad Request");
    }
    else if (line.substring(0, 25) == "HTTP/1.1 401 Unauthorized") {
      spn("HTTP : Unauthorized");
    }
    //addLog(LOG_LEVEL_DEBUG, log);
    delay(1);
  }
  spn("HTTP : closing connection");
  
  client.flush();
  client.stop();
}



