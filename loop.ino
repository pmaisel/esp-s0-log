// ########################################################################################
// ##### L O O P ##########################################################################
// ########################################################################################

void loop()
{
  flashLed(false);
  
  if (millis() - waitTime > 1000) 
  {
    waitTime = millis();
    
    // status LED
    int state = digitalRead(LED_BUILTIN); 
    digitalWrite(LED_BUILTIN, !state);
    
    // every minute
    if( second() == 0 )
    {
       // runtime
       ausgabe[9].value++;
       dtostrf(ausgabe[9].value, 2, 0, ausgabe[9].printValue);
    
      // check summertime
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

      if (minute() % 5 == 0)
      {
        //write2logfile("Test fuenf Minuten Schleife");

        /* begin neu */
        today.year  = year(t); 
        today.month = month(t)-1;  // -1 wegen dem Array das mit Null beginnt
        today.day   = day(t)-1;    // -1 wegen dem Array das mit Null beginnt
        
        // Hat ein neuer Tag begonnen?
        if( hour(t) == 0 && minute(t) == 0 ) 
        {              
          FuenfMinutenTimer(yesterday.month, yesterday.day); // sichere die letzten fünf Minuten in den letzten Tag
          putEeprom(yesterday.month, yesterday.day);
          CreateSensorString (); // Daten für das senden an FHEM vorbereiten
          
          // für den neuen Tag vorbereiten
          for (int i = 0; i < (sizeof(fuenfMinutenZaehler)/sizeof(int)); i++) 
            fuenfMinutenZaehler[i] = 0; // Tagescounter im RAM löschen
            
          ausgabe[0].value = 0; // Tagesertrag reset
          ausgabe[5].value = 0; // Maximalwert reset
      
          // Hat ein neuer Monat begonnen?
          if (today.day == 0)
          { 
            for (byte i = 0; i <= anzahlTage; ++i) 
              tagesCounter[today.month][i] = 0;
            ausgabe[1].value = 0; 
          }
    
          // Hat ein neues Jahr begonnen?
          if (today.month == 0)
          {
            backup2file ();   // Ertragsdaten Array in das SPPIF sichern
            deleteEEPROM ();  
            ausgabe[2].value = 0;
          }
        } // if (hour() == 0 && minute() == 0)
        else
        {
          FuenfMinutenTimer(today.month, today.day);
          CreateSensorString (); // Daten an den FHEM Controller senden
          
          if( hour(t) == 23 && minute(t) == 55 )
          {
            yesterday.year = today.year;
            yesterday.month = today.month;
            yesterday.day = today.day;
          }
        
        } // else (hour() == 0 && minute() == 0)
  
        dtostrf((ESP.getVcc()/1000.0), 2, 2, ausgabe[7].printValue);
        dtostrf(WiFi.RSSI(), 2, 0, ausgabe[8].printValue);
        /* ende neu */
      }
    }
  }
  
  // ##### Impuls eingegangen
  if (tickDetected)
  {
    flashLed(true); // Zeigt den empfangenen Impuls an
    tickDetected = false;
  }
 
  server.handleClient();
}
