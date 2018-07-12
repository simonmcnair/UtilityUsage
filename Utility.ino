

// This is for an Amica NodeMCU so some of the LED logic is reversed.


ADC_MODE(ADC_VCC);

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
//#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <TimeLib.h>
#include <ESP8266HTTPClient.h>

// #define DEBUG 1


const char *ssid = "####";
const char *password = "####";

unsigned long mydatetime;
unsigned int localPort = 2390;      // local port to listen for UDP packets
long debouncing_time = 100; //Debouncing Time in Milliseconds

//Elec Variables
const byte  Elec_LDRPin = 4; //D2
const float Elec_StandingCharge = 18.375;
const float Elec_UnitCharge = 11.760;
const int   Elec_LDRBlinksPerKWH = 1000;

volatile float         PowerKW = 0;
volatile unsigned long NumberOfKWH = 0;
volatile unsigned long Elec_TimeLastblinked = 0;
volatile int           Elec_LDRBlink = 0;
volatile bool          Elec_Meterblinked = false;

//Gas Variables
volatile float         GASflashrate = 0;
volatile unsigned long GASTimeLastblinked = 0;
volatile float         GasCharge = 0;
volatile int           GASLDRBlink = 0;
volatile bool          GASMeterblinked = false;

// 1 pulse every 10dm3 = .01m3
// 1000 pulses = 1m3 = 11kWh
const float            GasStandingCharge = 18.9;
const float            GasUnitCharge = 2.756; //1dm3 is equal to 11.1868KWH, 11.36266
const byte             GASLDRPin = 5; //D1
const char				GaSRemoteServer = "Dellboy";
const char				GaSRemoteServerURL;
const char				GaSRemoteServer;
const char				GaSRemoteServer;

const int            led = 2;
int                  ledState = LOW;             // ledState used to set the LED
unsigned long        previousMillis = 0;        // will store last time LED was updated
const long           interval = 1000;           // interval at which to blink (milliseconds)

ESP8266WebServer server ( 80 );


time_t getNtpTime();
void digitalClockDisplay();
String printDigits(int digits);
void sendNTPpacket(IPAddress &address);


/* Don't hardwire the IP address or we won't get the benefits of the pool.
    Lookup the IP address for the host name instead */
IPAddress timeServerIP; // time.nist.gov NTP server address
const char* ntpServerName = "pool.ntp.org";
const int timeZone = 1;
const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message

byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets

// A UDP instance to let us send and receive packets over UDP
WiFiUDP udp;

void handleRoot()
{
  //Serial.println(String(micros()) + "Http connection requested");
  //char temp[400];
  long sec = millis() / 1000;
  long min = sec / 60;
  long hr = min / 60;
  long days = hr / 24;

  String  myresult =  " <html><head><meta http-equiv='refresh' content='5'/><title>Gas and Electric Meter</title><style>body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }";
  myresult += "</style></head><body><h1>Gas and Electric Meter Readings</h1><br>";
  //myresult += "<img src=/test.svg /><br>";
  myresult += "Uptime: " + String(days) + " days.  " + String(printDigits(hr % 24)) + ":" + String(printDigits(min % 60)) + ":" + String(printDigits(sec % 60)) + "<br>";
  myresult += "Time: " + String(digitalClock()) + "<br>";
  myresult += "Date: " + String(digitalDate()) + "<br>";
  myresult += "ESP ChipId: " + String(ESP.getChipId()) + "<br>";
  myresult += "ESP ChipSize: " + String(ESP.getFlashChipSize()) + "<br>";
  myresult += "KWH: " + String(NumberOfKWH) + "." + String(Elec_LDRBlink) + "<BR>";
  float Currentcost = Elec_StandingCharge + ((NumberOfKWH + (float)(Elec_LDRBlink / 1000.0)) * Elec_UnitCharge);
  myresult += "ElectricityCost = " + String(Currentcost, 6) + "<br>";
  myresult += "Power Usage (Watts): " + String(PowerKW * 1000) + "<BR>";
  myresult += "Gas Meter count: " + String(GASLDRBlink) + "<BR>";
  myresult += "https://standards.globalspec.com/std/1329584/din-43864<BR>" ;

  float voltage = ESP.getVcc();
  myresult += "Voltage: " + String(voltage) + "V<br>";
  myresult += "</body></html>";

  server.send ( 200, "text/html", myresult );
  //myresult =  ;
}

void handleNotFound()
{
  //digitalWrite ( led, 1 );
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += ( server.method() == HTTP_GET ) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for ( uint8_t i = 0; i < server.args(); i++ )
  {
    message += " " + server.argName ( i ) + ": " + server.arg ( i ) + "\n";
  }
  server.send ( 404, "text/plain", message );
  //digitalWrite ( led, 0 );
}



void GASLDRfunction()
{
  unsigned long GASnewTime = micros() - GASTimeLastblinked;
  if (GASnewTime >= debouncing_time * 1000)
  {
    GASLDRBlink++;
    Serial.println(":  GASLDR Blink Count: " + String(GASLDRBlink));

    GASflashrate = float((micros() - GASTimeLastblinked));
    Serial.println(String(micros()) + "GAS LDRcounter = Elec_LDRBlinksPerKWH reset counter");

    GASTimeLastblinked = micros();
    GASMeterblinked = true;
  }
  else
  {
    Serial.println("GAS Debounce: Last blinked at " + String(GASTimeLastblinked) + "Ms. Millis is now: " + String((long)micros()) + "Ms. Difference is " + (long)(micros() - GASTimeLastblinked) + "ms. debounce period is " + String(debouncing_time * 1000));
  }
}

void ElecLDRfunction()
{
  unsigned long Elec_newTime = micros() - Elec_TimeLastblinked;

  //   Serial.println("Elec_newtime:" + String(Elec_newTime));

  if (Elec_newTime >= debouncing_time * 1000)
  {
    Elec_LDRBlink++;
    Serial.println(":  Elec_LDR Blink Count: " + String(Elec_LDRBlink));


    if (Elec_newTime > 0)
    {
      PowerKW = (3600 / (float)Elec_newTime); //Power (kW) = 3600 (secs in 1hr) divided by (the seconds between flashes * number of Imp/kWh printed on meter)
      //Serial.println("powerKW:" + String(PowerKW));
      PowerKW = PowerKW * Elec_LDRBlinksPerKWH;
      //Serial.println("powerKW:" + String(PowerKW));
      Serial.println("Elec Flashrate interval = " + String((float)Elec_newTime / 1000000) + "s.  Power consumption = " + String(PowerKW * 1000) + "W (" + String(PowerKW) + "KW.)  KWH used: " + String(NumberOfKWH) );
    }
    if (Elec_LDRBlink == Elec_LDRBlinksPerKWH)
    {
      Elec_LDRBlink = 0;
      NumberOfKWH++;
      Serial.println(String(micros()) + " Elec_LDRcounter = Elec_LDRBlinksPerKWH reset counter");
    }
    Elec_TimeLastblinked = micros();
    Elec_Meterblinked = true;
  }
  else
  {
    Serial.println("Elec Debounce: Last blinked at " + String(Elec_TimeLastblinked) + "Ms. Millis is now: " + String((long)micros()) + "Ms. Difference is " + (long)(micros() - Elec_TimeLastblinked) + "ms. debounce period is " + String(debouncing_time * 1000) + "ms");
  }
}

void setup ()
{
  interrupts();
  sei ();

  pinMode ( led, OUTPUT );
  pinMode ( LED_BUILTIN, OUTPUT );


  pinMode(GASLDRPin, INPUT_PULLUP); //D0
  attachInterrupt(digitalPinToInterrupt(GASLDRPin), GASLDRfunction, CHANGE);

  pinMode(Elec_LDRPin, INPUT_PULLUP); //D1
  attachInterrupt(digitalPinToInterrupt(Elec_LDRPin), ElecLDRfunction, CHANGE);


  Serial.begin ( 115200 );
  //     digitalWrite ( led, 0 );
  WiFi.mode ( WIFI_STA );
  WiFi.begin ( ssid, password );
  Serial.println ( "Waiting For WiFi " );
  // Wait for connection
  while ( WiFi.status() != WL_CONNECTED )
  {
    delay ( 500 );
    Serial.print ( "." );
  }
  Serial.println ("");
  Serial.println ( "Connected to ");
  Serial.println ( ssid );
  Serial.println ( "IP address: " );
  Serial.println ( WiFi.localIP() );

  //Serial.println ( "DNS to ");
  //Serial.println ( String(WiFi.dns1) );


  WiFi.printDiag(Serial);
  // if ( MDNS.begin ( "esp8266" ) )
  // {
  //   Serial.println ( "MDNS responder started" );
  // }
  Serial.println("Starting UDP");
  udp.begin(localPort);
  Serial.print("Local port: ");
  Serial.println(udp.localPort());

  setSyncProvider(getNtpTime);
  setSyncInterval(300);

  server.on ( "/", handleRoot );
  //server.on ( "/test.svg", drawGraph );
  server.on ( "/inline", []() {
    server.send ( 200, "text/plain", "this works as well" );
  } );
  server.onNotFound ( handleNotFound );
  server.begin();
  Serial.println ( "HTTP server started" );

  //   digitalWrite ( led, 1 );
}

time_t prevDisplay = 0; // when the digital clock was displayed


void loop ()
{
  server.handleClient();
  //Yes I do Serial.print("do I ever get here ? ");
  //Serial.println("Result : " + getNtpTime());
  if (timeStatus() != timeNotSet)
  {
    if (now() != prevDisplay)
    { //update the display only if time has changed
      // digitalWrite ( led, 0 );
      prevDisplay = now();
      digitalClock();
      // digitalWrite ( led, 1 );
    }
  }

  // Electric Meter counter
  if (Elec_Meterblinked == true) {
    Serial.println("Started Posting Electric Meter");
    digitalWrite ( LED_BUILTIN, LOW );

    HTTPClient http;
    http.begin("http://192.168.1.240:80/GasAndElectric/Elec.php?ElecReading=1");

    int httpCode = http.GET();
    String payload = http.getString();
    if (httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
      Serial.printf("[HTTP] GET... code: %d\n", httpCode);

      // file found at server
      if (httpCode == HTTP_CODE_OK) {
        //    String payload = http.getString();
        //Serial.println(payload);
        Serial.println("Http ok");
      }
    } else {
      Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }
    //Serial.println(payload);    //Print request response payload

    http.end();
    Elec_Meterblinked = false;
    digitalWrite ( LED_BUILTIN, HIGH );
    Serial.println("Finished Posting Electric Meter");
  }
  // Gas Meter Counter

  if (GASMeterblinked == true) {
    Serial.println("Started Posting Gas Meter");
    //digitalWrite ( LED_BUILTIN, LOW );

    HTTPClient http;
    http.begin("http://192.168.1.240:80/GasAndElectric/Gas.php?GasReading=1");

    int httpCode = http.GET();
    String payload = http.getString();
    if (httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
      Serial.printf("[HTTP] GET... code: %d\n", httpCode);

      // file found at server
      if (httpCode == HTTP_CODE_OK) {
        //    String payload = http.getString();
        Serial.println(payload);
      }
    } else {
      Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }
    Serial.println(payload);    //Print request response payload

    http.end();
    GASMeterblinked = false;
    Serial.println("Finished Posting Gas Meter");
    //digitalWrite ( LED_BUILTIN, HIGH );
  }


  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    // save the last time you blinked the LED
    previousMillis = currentMillis;

    // if the LED is off turn it on and vice-versa:
    if (ledState == LOW) {
      ledState = HIGH;
    } else {
      ledState = LOW;
    }

    // set the LED with the ledState of the variable:
    digitalWrite(led, ledState);
  }
}

String digitalClock()
{
  // digital clock display of the time
  String result;
  result = printDigits(hour()) + ":" + printDigits(minute()) + ":" + printDigits(second());
  return result;
}

String digitalDate()
{
  // digital clock display of the time
  String result;
  result = printDigits((day())) + "/"  + printDigits((month())) + "/" + String(year());
  return result;
}

String printDigits(int digits)
{
  // utility for digital clock display: prints preceding colon and leading 0


  String result = "";

  if (digits < 10)
  {
    result = "0" + String(digits);
  }
  else {
    result = String(digits);
  }

  //char mychar[2] = "";
  //sprintf(mychar,"%02d",digits);
  //result = String(mychar);

  return result;
}

time_t getNtpTime()
{
  IPAddress ntpServerIP; // NTP server's ip address
  //const int SECS_PER_HOUR = 3600;
  while (udp.parsePacket() > 0) ; // discard any previously received packets
  Serial.println("Transmit NTP Request");
  // get a random server from the pool
  WiFi.hostByName(ntpServerName, ntpServerIP);
  Serial.print(ntpServerName);
  Serial.print(": ");
  Serial.println(ntpServerIP);
  sendNTPpacket(ntpServerIP);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500)
  {
    int size = udp.parsePacket();
    if (size >= NTP_PACKET_SIZE)
    {
      Serial.println("Receive NTP Response");
      udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      Serial.println("Epochtime:" + String(secsSince1900));
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
      Serial.println("Massagedtime:" + String(secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR));
    }
  }
  Serial.println("No NTP Response :-(");
  return 0; // return 0 if unable to get the time
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  udp.beginPacket(address, 123); //NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}
