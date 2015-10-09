/*--------------------------------------------------------------
  use: 
  Program:      eth_websrv_SD_Ajax_in_out
  Description:  Arduino web server that displays 4 analog inputs,
                the state of 3 switches and controls 4 outputs,
                2 using checkboxes and 2 using buttons.
                The web page is stored on the micro SD card.
  
  Hardware:     Arduino Uno and official Arduino Ethernet
                shield. Should work with other Arduinos and
                compatible Ethernet shields.
                2Gb micro SD card formatted FAT16.
                A2 to A4 analog inputs, pins 2, 3 and 5 for
                the switches, pins 6 to 9 as outputs (LEDs).
                
  Software:     Developed using Arduino 1.0.5 software
                Should be compatible with Arduino 1.0 +
                SD card contains web page called index.htm
  
  References:   - WebServer example by David A. Mellis and 
                  modified by Tom Igoe
                - SD card examples by David A. Mellis and
                  Tom Igoe
                - Ethernet library documentation:
                  http://arduino.cc/en/Reference/Ethernet
                - SD Card library documentation:
                  http://arduino.cc/en/Reference/SD
  Date:         4 April 2013
  Modified:     19 June 2013
                - removed use of the String class
 
  Author:       W.A. Smith, http://startingelectronics.com
  
  adapted sketch by niq_ro from http://www.tehnic.go.ro
  http://nicuflorica.blogspot.ro
  http://arduinotehniq.blogspot.com/
  webthermostat ver.version. 6.3c3 / 09.10.2015
--------------------------------------------------------------*/

#include <SPI.h>
#include <Ethernet.h>
#include <SD.h>
// size of buffer used to capture HTTP requests
#define REQ_BUF_SZ   60

// MAC address from Ethernet shield sticker under board
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192, 168, 2, 199); // IP address, may need to change depending on network
byte subnet[] = { 255, 255, 255, 0 };
byte gateway[] = { 192, 168, 2, 1 };    

EthernetServer server(8081);  // create a server at port 80
File webFile;               // the web page file on the SD card
char HTTP_req[REQ_BUF_SZ] = {0}; // buffered HTTP request stored as null terminated string
char req_index = 0;              // index into HTTP_req buffer
boolean LED_state[5] = {0}; // stores the states of the LEDs

float t,h;        // value for temperature and humidity
#include "DHT.h"
#define DHTPIN A2     // what pin we're connected to
// Uncomment whatever type you're using!
//#define DHTTYPE DHT11   // DHT 11 
#define DHTTYPE DHT22   // DHT 22  (AM2302)
//#define DHTTYPE DHT21   // DHT 21 (AM2301)
// Connect pin 1 (on the left) of the sensor to +5V
// Connect pin 2 of the sensor to whatever your DHTPIN is
// Connect pin 4 (on the right) of the sensor to GROUND
// Connect a 10K resistor from pin 2 (data) to pin 1 (power) of the sensor
DHT dht(DHTPIN, DHTTYPE);

// http://tronixstuff.com/2011/03/16/tutorial-your-arduinos-inbuilt-eeprom/
#include <EEPROM.h>

int stare; // 0 is OFF, 1 is ON
int stare0; // before state
//int teset = 225;  // temperaturee set x 10;
//int dete = 5;   // hysteresys temperature x 10;

float teset, dete;
int teset0, teset1, teset2, teset3;
int numere = 0;

void setup()
{
    // disable Ethernet chip
    pinMode(10, OUTPUT);
    digitalWrite(10, HIGH);
    
     dht.begin();              // initialize DHT sensor
    
    Serial.begin(9600);       // for debugging
    
    // initialize SD card
    Serial.println("Initializing SD card...");
    if (!SD.begin(4)) {
        Serial.println("ERROR - SD card initialization failed!");
        return;    // init failed
    }
    Serial.println("SUCCESS - SD card initialized.");
    // check for index.htm file
    if (!SD.exists("index.htm")) {
        Serial.println("ERROR - Can't find index.htm file!");
        return;  // can't find index file
    }
    Serial.println("SUCCESS - Found index.htm file.");
    // switches on pins 2, 3 and 5
 //   pinMode(2, INPUT);
 //   pinMode(3, INPUT);
 //   pinMode(5, INPUT);
    // LEDs
 //   pinMode(6, OUTPUT);
 //   pinMode(7, OUTPUT);
 //   pinMode(8, OUTPUT);
    pinMode(9, OUTPUT);
    
 //   Ethernet.begin(mac, ip);  // initialize Ethernet device
 //   server.begin();           // start to listen for clients

 // start the Ethernet connection and the server:
  Ethernet.begin(mac, ip, gateway, subnet);
  server.begin();
  
Serial.print("server is at ");
Serial.println(Ethernet.localIP());

t = dht.readTemperature();
Serial.print("t =");
Serial.print(t);
Serial.println("^C");

//teset = 22.5;
//dete = 0.5;

/*
EEPROM.write(150,205);
EEPROM.write(149,0);
EEPROM.write(151,5);
*/

teset1 = EEPROM.read(150);
teset0 = EEPROM.read(149);
teset = 256*teset0+teset1;
dete = EEPROM.read(151);

LED_state[4] = 1; // thermostat is on AUTO mode
}

void loop()
{
    EthernetClient client = server.available();  // try to get client

    if (client) {  // got client?
        boolean currentLineIsBlank = true;
        while (client.connected()) {
            if (client.available()) {   // client data available to read
                char c = client.read(); // read 1 byte (character) from client
                // limit the size of the stored received HTTP request
                // buffer first part of HTTP request in HTTP_req array (string)
                // leave last element in array as 0 to null terminate string (REQ_BUF_SZ - 1)
                if (req_index < (REQ_BUF_SZ - 1)) {
                    HTTP_req[req_index] = c;          // save HTTP request character
                    req_index++;
                }
                // last line of client request is blank and ends with \n
                // respond to client only after last line received
                if (c == '\n' && currentLineIsBlank) {
                    // send a standard http response header
                    client.println("HTTP/1.1 200 OK");
                    // remainder of header follows below, depending on if
                    // web page or XML page is requested
                    // Ajax request - send XML file
                    if (StrContains(HTTP_req, "ajax_inputs")) {
                        // send rest of HTTP header
                        client.println("Content-Type: text/xml");
                        client.println("Connection: keep-alive");
                        client.println();
                        SetLEDs();
                        // send XML file containing input states
                        XML_response(client);
                    }
                    else {  // web page request
                        // send rest of HTTP header
                        client.println("Content-Type: text/html");
                        client.println("Connection: keep-alive");
                        client.println();
                        // send web page
                        webFile = SD.open("index.htm");        // open web page file
                        if (webFile) {
                            while(webFile.available()) {
                                client.write(webFile.read()); // send web page to client
                            }
                            webFile.close();
                        }
                    }
                    // display received HTTP request on serial port
                    Serial.print(HTTP_req);
                    // reset buffer index and all buffer elements to 0
                    req_index = 0;
                    StrClear(HTTP_req, REQ_BUF_SZ);
                    break;
                }
                // every line of text received from the client ends with \r\n
                if (c == '\n') {
                    // last character on line of received text
                    // starting new line with next character read
                    currentLineIsBlank = true;
                } 
                else if (c != '\r') {
                    // a text character was received from client
                    currentLineIsBlank = false;
                }
            } // end if (client.available())
        } // end while (client.connected())
        delay(1);      // give the web browser time to receive the data
        client.stop(); // close the connection
    } // end if (client)
    
 // if isn't client
 numere = numere + 1;
delay(1);
if (numere > 32000) {
t = dht.readTemperature();
numere = 0;
Serial.print("t =");
Serial.print(t);
Serial.println("^C");
}
// read switches
if ((LED_state[4] == 1) and (t > teset/10)) {
  digitalWrite(9, LOW);
}  
if ((LED_state[4] == 1) and (t < teset/10 - dete/10)) {
  digitalWrite(9, HIGH);
}  
if (LED_state[4] == 0) {
  digitalWrite(9, LOW);
}  
 
    
}

// checks if received HTTP request is switching on/off LEDs
// also saves the state of the LEDs
void SetLEDs(void)
{
    // LED 1 (pin 6)
    if (StrContains(HTTP_req, "LED1=1")) {
      teset = teset + 5;
      LED_state[0] = 1;  // save LED state
  //      digitalWrite(6, HIGH);
// calculate the unitar and zecinal units
teset0 = teset/256;
teset1 = teset - teset0*256;
// write in internal eeprom
EEPROM.write(150,teset1);
EEPROM.write(149,teset0);
    }
    else if (StrContains(HTTP_req, "LED1=0")) {
      teset = teset + 5;  
      LED_state[0] = 0;  // save LED state
 //       digitalWrite(6, LOW);
 // calculate the unitar and zecinal units
teset0 = teset/256;
teset1 = teset - teset0*256;
// write in internal eeprom
EEPROM.write(150,teset1);
EEPROM.write(149,teset0);
    }
    // LED 2 (pin 7)
    if (StrContains(HTTP_req, "LED2=1")) {
      teset = teset - 5;
      LED_state[1] = 1;  // save LED state
  //      digitalWrite(7, HIGH);
// calculate the unitar and zecinal units
teset0 = teset/256;
teset1 = teset - teset0*256;
// write in internal eeprom
EEPROM.write(150,teset1);
EEPROM.write(149,teset0);
    }
    else if (StrContains(HTTP_req, "LED2=0")) {
      teset = teset - 5;
      LED_state[1] = 0;  // save LED state
  //      digitalWrite(7, LOW);
// calculate the unitar and zecinal units
teset0 = teset/256;
teset1 = teset - teset0*256;
// write in internal eeprom
EEPROM.write(150,teset1);
EEPROM.write(149,teset0);
    }
    // LED 3 (pin 8)
    if (StrContains(HTTP_req, "LED3=1")) {
        dete = dete + 1;
        if (dete > 20) dete = 20;
        LED_state[2] = 1;  // save LED state
 //       LED_state[2] = 0;  // save LED state
//        digitalWrite(8, HIGH);
    }
    else if (StrContains(HTTP_req, "LED3=0")) {
        dete = dete + 1;
        if (dete > 20) dete = 20;
  EEPROM.write(151,dete);
        LED_state[2] = 0;  // save LED state
 //       LED_state[2] = 1;  // save LED state     
        }
    // LED 4 (pin 9)
    if (StrContains(HTTP_req, "LED4=1")) {
        dete = dete - 1;
   EEPROM.write(151,dete);       
        if (dete < 1) dete = 1;
        LED_state[3] = 1;  // save LED state
  //      digitalWrite(9, HIGH);
    }
    else if (StrContains(HTTP_req, "LED4=0")) {
        dete = dete - 1;
        if (dete < 1) dete = 1;
        LED_state[3] = 0;  // save LED state
  //      digitalWrite(9, LOW);
    }
    // LED 5
    if (StrContains(HTTP_req, "LED5=1")) {
        LED_state[4] = 1;  // save LED state
  //      digitalWrite(9, HIGH);
   //     if (t > teset) digitalWrite(9, LOW);
   //     if (t < teset - dete) digitalWrite(9, HIGH);
    }
    else if (StrContains(HTTP_req, "LED5=0")) {
        LED_state[4] = 0;  // save LED state
  //      digitalWrite(9, LOW);
    }

}

// send the XML file with analog values, switch status
//  and LED status
void XML_response(EthernetClient cl)
{
    int analog_val;            // stores value read from analog inputs
    
    cl.print("<?xml version = \"1.0\" ?>");
    cl.print("<inputs>");
    // read analog inputs
        t = dht.readTemperature();
        cl.print("<analog>");
        cl.print(t,1);
        cl.println("</analog>");
        h = dht.readHumidity();
        cl.print("<analog>");
        cl.print(h,1);
        cl.println("</analog>");

        cl.print("<analog>");
        cl.print(teset/10,1);
        cl.println("</analog>");
        cl.print("<analog>");
        cl.print(dete/10,1);
        cl.println("</analog>");
/*
// read switches
    cl.print("<switch>");
        if (digitalRead(9)) {
            cl.print("ON");
        }
        else {
            cl.print("OFF");
        }
        cl.println("</switch>");    
 */
 // button LED states
  // LED1
    cl.print("<LED>");
    if (LED_state[0]) {
        cl.print("on");
    }
    else {
        cl.print("off");
    }
    cl.println("</LED>");
  // LED2
    cl.print("<LED>");
    if (LED_state[1]) {
        cl.print("on");
    }
    else {
        cl.print("off");
    }
    cl.println("</LED>");
  // LED3
    cl.print("<LED>");
    if (LED_state[2]) {
        cl.print("on");
    }
    else {
        cl.print("off");
    }
    cl.println("</LED>");
  // LED4
    cl.print("<LED>");
    if (LED_state[3]) {
        cl.print("on");
    }
    else {
        cl.print("off");
    }
    cl.println("</LED>");
  // LED5
    cl.print("<LED>");
    if (LED_state[4]) {
        cl.print("on");
    }
    else {
        cl.print("off");
    }
    cl.println("</LED>");

// read switches
    cl.print("<switch>");
    //    if (digitalRead(9)) {
if ((LED_state[4] == 1) and (t > teset/10)) {
  digitalWrite(9, LOW);
//  cl.print("<font color=red>");
  cl.print("It's warm..");
}  
if ((LED_state[4] == 1) and (t < teset/10 - dete/10)) {
  digitalWrite(9, HIGH);
//   cl.print("<font color=blue>");
          cl.print("It's cold..");
}  
if ((LED_state[4] == 1) and (t >= teset/10 - dete/10)) {
  if (t <=teset/10) {
//  cl.print("<font color=green>");
  cl.print("It's ok..");
}  
}
//cl.print("</font>");

if (LED_state[4] == 0) {
  digitalWrite(9, LOW);
          cl.print("OFF manual");
}  
      if (digitalRead(9)) {
   //         cl.print("<font color=red>");
            cl.print("-> heater on");
        }
        else {
   //         cl.print("<font color=blue>");
            cl.print("-> heater is off");
        }
        cl.println("</switch>");    
    cl.print("</inputs>"); 
}

// sets every element of str to 0 (clears array)
void StrClear(char *str, char length)
{
    for (int i = 0; i < length; i++) {
        str[i] = 0;
    }
}

// searches for the string sfind in the string str
// returns 1 if string found
// returns 0 if string not found
char StrContains(char *str, char *sfind)
{
    char found = 0;
    char index = 0;
    char len;

    len = strlen(str);
    
    if (strlen(sfind) > len) {
        return 0;
    }
    while (index < len) {
        if (str[index] == sfind[found]) {
            found++;
            if (strlen(sfind) == found) {
                return 1;
            }
        }
        else {
            found = 0;
        }
        index++;
    }

    return 0;
}
