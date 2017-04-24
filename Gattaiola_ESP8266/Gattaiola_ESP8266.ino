#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <TimeLib.h>

#define UART_BAUD 2400

// ESP WiFi mode:
//#define MODE_AP // phone connects directly to ESP
#define MODE_STA // ESP connects to router


#ifdef MODE_AP
// For AP mode:
const char *ssid = "mywifi";  // You will connect your phone to this Access Point
const char *pw = "qwerty123"; // and this is the password
IPAddress ip(192, 168, 0, 1); // From RoboRemo app, connect to this IP
IPAddress netmask(255, 255, 255, 0);
const int port = 9876; // and this port
// You must connect the phone to this AP, then:
// menu -> connect -> Internet(TCP) -> 192.168.0.1:9876
#endif


#ifdef MODE_STA
// For STATION mode:
const char *ssid = "USR9111";  // Your ROUTER SSID
const char *pw = "bruusroboticsbru"; // and WiFi PASSWORD
const int port = 9876;
IPAddress ip(192,168,2,91);  //Node static IP
IPAddress gateway(192,168,2,1);
IPAddress subnet(255,255,255,0);
// You must connect the phone to the same router,
// Then somehow find the IP that the ESP got from router, then:
// menu -> connect -> Internet(TCP) -> [ESP_IP]:9876
#endif

ESP8266WebServer server(port);
String mainPage = "<html><head></head><body><iframe frameBorder='0' height='400' width='600' id='static-content' src='staticPage'></iframe><br><iframe frameBorder='0' id='dynamic-content' src='refresh.html'></iframe></body></html>";
String staticPage = "<html><head><style type='text/css'>.doorButton{height:200px;width:200px;text-align:center;font-size:24px;}</style></head><body><button class='doorButton' style='background-color:green' onclick=\"window.location.href='/staticPage?status=open'\">OPEN</button><button class='doorButton' style='background-color:red' onclick=\"window.location.href='/staticPage?status=close'\">CLOSE</button><br><br><form action='/staticPage'>Set open time: <input type='number' name='setAlarmHour' min='0' max='23' value='6'/> - <input type='number' name='setAlarmMinute' min='0' max='59' value='0'/><br><input type='submit' value='Submit'/></form><br><form action='/staticPage'><input type='hidden' name='settime' id='settime' value=''/><input type='submit' value='Set Time'/></form><script type='text/javascript'>document.getElementById('settime').value=setTime();function setTime(){var d=new Date();return Math.round(d.getTime()/1000);}</script></body></html>";
//String staticPage = "Static";
//String refreshPage = "Refresh";

bool stringComplete = false;
String arduinoAnswer = "";
String status_color = "blue";
String arduino_time = "";
int openHour;
int openMinute;

//////////////////////////////////////////////////
void setup() {
  delay(100);
  Serial.begin(UART_BAUD);
  delay(100);
  Serial.println("Starting...");

  #ifdef MODE_AP 
  //AP mode (phone connects directly to ESP) (no router)
  WiFi.softAPConfig(ip, ip, netmask); // configure ip address for softAP 
  WiFi.softAP(ssid, pw); // configure ssid and password for softAP
  #endif

  
  #ifdef MODE_STA
  // STATION mode (ESP connects to router and gets an IP)
  // Assuming phone is also connected to that router
  // from RoboRemo you must connect to the IP of the ESP
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pw);
  WiFi.config(ip, gateway, subnet);
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("Connected to:");
  Serial.println(ssid);
  
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  #endif

  server.on("/", handleMain);
  server.on("/staticPage", handleGenericArgsStatic);
  server.on("/refresh.html", handleRefreshIframe);
  
  server.begin(); // start TCP server
  Serial.println("HTTP server started");
}

void handleRefreshIframe() {
  String command = "AT!STATUS?\r\n";
  Serial.println(command);
  command = "AT!TIME?\r\n";
  Serial.println(command);
  command = "AT!ALARM?\r\n";
  Serial.println(command);
  String refreshPage = "<html><head><meta http-equiv='refresh' content='5'><style>div{background-color: white;width: 250px;}</style><script type='text/javascript'>function printLocalTime(){var d = new Date();document.write(d.toLocaleString());}</script></head><body bgcolor=" + status_color + "><div>Local Time: <script type='text/javascript'>printLocalTime();</script><br>Remote Time: " + arduino_time + "<br>Open Time: " + openHour + ":" + openMinute + "</div></body></html>";

  server.send(200, "text/html", refreshPage);       //Response to the HTTP request
}

void handleMain() {
  server.send(200, "text/html", mainPage);
}
void handleGenericArgsStatic() { //Handler
  String message = "Number of args received:";
  message += server.args();            //Get number of parameters
  message += "\n";                     //Add a new line

  for (int i = 0; i < server.args(); i++) {
    message += "Arg n " + (String)i + " -> "; //Include the current iteration value
    message += server.argName(i) + ": ";      //Get the name of the parameter
    message += server.arg(i) + "\n";          //Get the value of the parameter
  }
  Serial.println(message);
  
  if(server.argName(0).equalsIgnoreCase("STATUS"))
  {
    String command = "AT!STATUS=";
    command += (server.arg(0).equalsIgnoreCase("OPEN")?"OPEN":"CLOSE");
    Serial.println(command);
  }
  
  else if(server.argName(0).equalsIgnoreCase("SETTIME"))
  {
    String command = "AT!TIME=" + server.arg(0) + "\n";
    Serial.println(command);
  }

  else if(server.argName(0).equalsIgnoreCase("SETALARMHOUR"))
  {
    String command = "AT!ALARM=" + command += server.arg(0) + "," + server.arg(1) + "\n";
    Serial.println(command);
  }
  server.send(200, "text/html", staticPage);       //Response to the HTTP request
}

void loop() {
  server.handleClient();
  while (Serial.available())
  {
    char inChar = (char)Serial.read();
    arduinoAnswer += inChar;
    if (inChar == '\n')
    {
      stringComplete = true;
    }
  }
  
  if (stringComplete)
  {
    arduinoAnswer.trim();
    arduinoAnswer.toUpperCase();
    
    if(arduinoAnswer.startsWith("STATUS="))
    {
      if (arduinoAnswer.substring(7) == "CLOSE")
        status_color = "red";
      else if (arduinoAnswer.substring(7) == "OPEN")
        status_color = "green";
      else if (arduinoAnswer.substring(7) == "OPENING")
        status_color = "yellow";
      else if (arduinoAnswer.substring(7) == "CLOSING")
        status_color = "yellow";
      else
        status_color = "blue";
    }
    else if(arduinoAnswer.startsWith("ALARM="))
    {
      openHour = arduinoAnswer.substring(arduinoAnswer.indexOf('=') + 1, arduinoAnswer.indexOf(':')).toInt();
      Serial.println(arduinoAnswer.substring(arduinoAnswer.indexOf('=') + 1, arduinoAnswer.indexOf(':')));
      Serial.println(openHour);
      openMinute = arduinoAnswer.substring(arduinoAnswer.indexOf(':') + 1).toInt();
      Serial.println(arduinoAnswer.substring(arduinoAnswer.indexOf(':') + 1));
      Serial.println(openMinute);
    }
    else if(arduinoAnswer.startsWith("TIME="))
    {
      int myTime = arduinoAnswer.substring(5).toInt();
      arduino_time = day(myTime);
      arduino_time += "/";
      arduino_time += month(myTime);
      arduino_time += "/";
      arduino_time += year(myTime);
      arduino_time += ", ";
      arduino_time += hour(myTime);
      arduino_time += ":";
      arduino_time += minute(myTime);
      arduino_time += ":";
      arduino_time += second(myTime);
    }
    arduinoAnswer = "";
    stringComplete = false;
  }
}

