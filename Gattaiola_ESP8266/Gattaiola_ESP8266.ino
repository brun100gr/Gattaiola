#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <DS3232RTC.h>    //http://github.com/JChristensen/DS3232RTC
#include <Time.h>         //http://www.arduino.cc/playground/Code/Time
#include <TimeLib.h>
#include <Wire.h>      //http://arduino.cc/en/Reference/Wire

//const char* ssid = "Vodafone-34176832";
//const char* password = "3z5cf5kaeavxf5a";

const char* ssid = "AndroidAP";
const char* password = "wwfk6438";

const int port = 9876;
ESP8266WebServer server(port);

bool stringComplete = false;
String incomingString = "";
String mainPage = "<html><head></head><body>Ciao</body></html>";

void setup_serial() {
  Serial.begin(115200);
  delay(100);
}

void setup_wifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void setup_OTA() {
  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  // ArduinoOTA.setPassword((const char *)"123");

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
}

void setup_RTC() {
    setSyncProvider(RTC.get);   // the function to get the time from the RTC
    if(timeStatus() != timeSet) {
        Serial.println("Unable to sync with the RTC");
    }else {
        Serial.println("RTC has set the system time");
    }
    
    bool stopped = RTC.oscStopped();
    if (stopped) {
        Serial.println("Detected clock power loss - resetting RTC date");
        // TODO: restore the date
    }
    else {
        Serial.println("Clock did not lose power");
    }
}

void setup_Server(void)
{
  server.on("/", handleMain);
  server.begin(); // start TCP server
  Serial.println("HTTP server started");

}

void setup() {
  setup_serial();
  setup_wifi();
  setup_OTA();
  setup_RTC();
  setup_Server();
}

void loop() {
  server.handleClient();
  ArduinoOTA.handle();
  digitalClockDisplay();
  delay(1000);
//  RTCMemoryDump();
  while (Serial.available())
  {
    char inChar = (char)Serial.read();
    incomingString += inChar;
    if (inChar == '\n')
    {
      stringComplete = true;
    }
  }

  if(stringComplete)
  {
    int value = incomingString.toInt();
    Serial.print("---->111: ");
    Serial.println(value);
    RTC.set(value);
    setSyncProvider(RTC.get);
    stringComplete = false;
  }
}


void digitalClockDisplay(void)
{
    // digital clock display of the time
    Serial.print(hour());
    printDigits(minute());
    printDigits(second());
    Serial.print(' ');
    Serial.print(day());
    Serial.print(' ');
    Serial.print(month());
    Serial.print(' ');
    Serial.print(year()); 
    Serial.println(); 
}
 
void printDigits(int digits)
{
    // utility function for digital clock display: 
    // prints preceding colon and leading 0
    Serial.print(':');
    if(digits < 10)
        Serial.print('0');
    Serial.print(digits);
}

void RTCMemoryDump(void)
{
  String dump = "RTC Memory Dump: ";
  Serial.print(dump);
  char tmp[16] = {0};
  for (int i = 0; i < 0x100; i++)
  {
    if(i%0x10 == 0)
    {
      sprintf(tmp, "0x%02X", i);
      dump += "\n  " + (String)tmp + "  ";
    }
    sprintf(tmp, "%02X ", RTC.readRTC(i));
    dump += (String)tmp;
  }
  Serial.println(dump);
}

void handleMain() {
  server.send(200, "text/html", mainPage);
}
