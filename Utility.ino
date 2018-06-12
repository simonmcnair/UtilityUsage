/*
   Copyright (c) 2015, Majenko Technologies
   All rights reserved.

   Redistribution and use in source and binary forms, with or without modification,
   are permitted provided that the following conditions are met:

 * * Redistributions of source code must retain the above copyright notice, this
     list of conditions and the following disclaimer.

 * * Redistributions in binary form must reproduce the above copyright notice, this
     list of conditions and the following disclaimer in the documentation and/or
     other materials provided with the distribution.

 * * Neither the name of Majenko Technologies nor the names of its
     contributors may be used to endorse or promote products derived from
     this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
   ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
   DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
   ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
   (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
   LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
   ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
ADC_MODE(ADC_VCC);

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <TimeLib.h>

const char *ssid = "asdad";
const char *password = "werwerrr";

unsigned long mydatetime;
unsigned int localPort = 2390;      // local port to listen for UDP packets
long debouncing_time = 250; //Debouncing Time in Milliseconds
volatile unsigned long last_micros;
volatile unsigned long NumberOfKWH = 0;
volatile unsigned long Timebetweenblinks = 0;
volatile unsigned long flashrate = 0;
volatile unsigned long TimeLastblinked = 0;
volatile float PowerKW = 0;

time_t getNtpTime();
void digitalClockDisplay();
String printDigits(int digits);
void sendNTPpacket(IPAddress &address);


/* Don't hardwire the IP address or we won't get the benefits of the pool.
    Lookup the IP address for the host name instead */
//IPAddress timeServer(129, 6, 15, 28); // time.nist.gov NTP server
IPAddress timeServerIP; // time.nist.gov NTP server address
const char* ntpServerName = "pool.ntp.org";
const int timeZone = 1;
const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message

byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets

// A UDP instance to let us send and receive packets over UDP
WiFiUDP udp;

ESP8266WebServer server ( 80 );

const int led = 13;
const byte ReedswitchPin = 5; //D1
const byte LDRPin = 4; //D2
const int LDRBlinksPerKWH = 1000;
int LDRBlink = 0;

void handleRoot() {
  digitalWrite ( led, 1 );
  //char temp[400];
  int sec = millis() / 1000;
  int min = sec / 60;
  int hr = min / 60;


  String  myresult =  " <html><head><meta http-equiv='refresh' content='5'/><title>ESP8266 Demo</title><style>body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }";
  myresult += "</style></head><body><h1>Hello Simon !</h1><img src=/test.svg /><br>";
  myresult += "Uptime: " + String(hr) + ":" + String(min) + ":" + String(sec) + "<br>";
  myresult += "Time: " + String(digitalClock()) + "<br>";
  myresult += "Date: " + String(digitalDate()) + "<br>";
  myresult += "ESP ChipId: " + String(ESP.getChipId()) + "<br>";
  myresult += "KWH: " + String(NumberOfKWH) + "<BR>";
  myresult += "LDR Blinks: " + String(LDRBlink) + "<BR>";

  float voltage = ESP.getVcc();
  myresult += "Voltage: " + String(voltage) + "V<br>";
  myresult += "</body></html>";


  server.send ( 200, "text/html", myresult );
  //myresult =  ;
  digitalWrite ( led, 0 );
}

void handleNotFound() {
  digitalWrite ( led, 1 );
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += ( server.method() == HTTP_GET ) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for ( uint8_t i = 0; i < server.args(); i++ ) {
    message += " " + server.argName ( i ) + ": " + server.arg ( i ) + "\n";
  }
  server.send ( 404, "text/plain", message );
  digitalWrite ( led, 0 );
}

time_t getNtpTime() {
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
  while (millis() - beginWait < 1500) {
    int size = udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
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

void Reedswitchfunction() {
  if ((long)(micros() - last_micros) >= debouncing_time * 1000) {
    Serial.println(printDigits(hour()) + ":" + printDigits(minute()) + ":" + printDigits(second()) + ": Reed Switch !");
    last_micros = micros();
  }
  else {
    Serial.println("Debounce");
  }
}

void LDRfunction() {
  if ((long)(micros() - last_micros) >= debouncing_time * 1000) {
    LDRBlink++;
    Serial.println(printDigits(hour()) + ":" + printDigits(minute()) + ":" + printDigits(second()) + ":  LDR !" + String(LDRBlink));

    flashrate = (micros() - TimeLastblinked) / LDRBlinksPerKWH;

    if (flashrate > 0) {
      PowerKW = 3600 / flashrate; //Power (kW) = 3600 (secs in 1hr) divided by (the seconds between flashes * number of Imp/kWh printed on meter)
      Serial.println("Flashrate = " + String(flashrate));
      Serial.println("PowerKW = " + String(PowerKW));
    }
    TimeLastblinked = micros();


    if (LDRBlink == LDRBlinksPerKWH) {
      LDRBlink = 1;
      NumberOfKWH++;
      Serial.println("LDRcounter = LDRBlinksPerKWH reset counter");
    }
    last_micros = micros();
  }
  else {
    Serial.println("Debounce");
  }
}

void setup () {
  interrupts();
  sei ();


  //pinMode ( led, OUTPUT );
  //digitalWrite ( led, 0 );

  pinMode(ReedswitchPin, INPUT_PULLUP); //D0
  attachInterrupt(digitalPinToInterrupt(ReedswitchPin), Reedswitchfunction, CHANGE);

  pinMode(LDRPin, INPUT_PULLUP); //D1
  attachInterrupt(digitalPinToInterrupt(LDRPin), LDRfunction, CHANGE);


  Serial.begin ( 115200 );
  WiFi.mode ( WIFI_STA );
  WiFi.begin ( ssid, password );
  Serial.println ( "" );
  // Wait for connection
  while ( WiFi.status() != WL_CONNECTED ) {
    delay ( 500 );
    Serial.print ( "." );
  }
  Serial.println ( "" );
  Serial.print ( "Connected to " );
  Serial.println ( ssid );
  Serial.print ( "IP address: " );
  Serial.println ( WiFi.localIP() );
  if ( MDNS.begin ( "esp8266" ) ) {
    Serial.println ( "MDNS responder started" );
  }
  Serial.println("Starting UDP");
  udp.begin(localPort);
  Serial.print("Local port: ");
  Serial.println(udp.localPort());
  setSyncProvider(getNtpTime);
  setSyncInterval(300);
  server.on ( "/", handleRoot );
  server.on ( "/test.svg", drawGraph );
  server.on ( "/inline", []() {
    server.send ( 200, "text/plain", "this works as well" );
  } );
  server.onNotFound ( handleNotFound );
  server.begin();
  Serial.println ( "HTTP server started" );
}

time_t prevDisplay = 0; // when the digital clock was displayed


void loop () {
  server.handleClient();
  //Serial.println("Result:" + getNtpTime());
  if (timeStatus() != timeNotSet) {
    if (now() != prevDisplay) { //update the display only if time has changed
      prevDisplay = now();
      digitalClock();
    }
  }
  //int ReedSwitch = 0;
  //ReedSwitch = digitalRead(16);
  //Serial.println("Reed Switch :" + String(ReedSwitch));

  //int LDR = 0;
  //LDR = digitalRead(5);
  //Serial.println("LDR :" + String(LDR));
}

String digitalClock() {
  // digital clock display of the time
  String result;
  result = printDigits(hour()) + ":" + printDigits(minute()) + ":" + printDigits(second());
  return result;
}

String digitalDate() {
  // digital clock display of the time
  String result;
  result = printDigits((day())) + "/"  + printDigits((month())) + "/" + String(year());
  return result;
}

String printDigits(int digits) {
  // utility for digital clock display: prints preceding colon and leading 0
  String result;
  if (digits < 10) {
    result = "0" + String(digits);
  }
  else {
    result = String(digits);
  }
  return result;
}




void drawGraph() {
  String out = "";
  char temp[100];
  out += "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\" width=\"400\" height=\"150\">\n";
  out += "<rect width=\"400\" height=\"150\" fill=\"rgb(250, 230, 210)\" stroke-width=\"1\" stroke=\"rgb(0, 0, 0)\" />\n";
  out += "<g stroke=\"black\">\n";
  int y = rand() % 130;
  for (int x = 10; x < 390; x += 10) {
    int y2 = rand() % 130;
    sprintf(temp, "<line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" stroke-width=\"1\" />\n", x, 140 - y, x + 10, 140 - y2);
    out += temp;
    y = y2;
  }
  out += "</g>\n</svg>\n";

  server.send ( 200, "image/svg+xml", out);
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address) {
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


