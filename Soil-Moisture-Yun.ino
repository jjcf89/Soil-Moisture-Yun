/*****************************************************************
Phant_Yun.ino
Post data to SparkFun's data stream server system (phant) using
an Arduino Yun
Jim Lindblom @ SparkFun Electronics
Original Creation Date: July 3, 2014

This sketch uses an Arduino Yun to POST sensor readings to 
SparkFun's data logging streams (http://data.sparkfun.com). A post
will be initiated whenever pin 3 is connected to ground.

Make sure you fill in your data stream's public, private, and data
keys before uploading! These are in the global variable section.

Hardware Hookup:
  * These components are connected to the Arduino's I/O pins:
    * D2 - DHT Data pin
    * D3-8 - Soil moisture power enable
    * A0-5 - Soil moisture readings
  * Your Yun should also, somehow, be connected to the Internet.
    You can use Ethernet, or the on-board WiFi module.

Development environment specifics:
    IDE: Arduino 1.6.10
    Hardware Platform: Arduino Yun

This code is beerware; if you see me (or any other SparkFun 
employee) at the local, and you've found our code helpful, please 
buy us a round!

curl example from:
https://github.com/sparkfun/phant/blob/master/examples/sh/curl_post.sh

Distributed as-is; no warranty is given.
*****************************************************************/
// Process.h gives us access to the Process class, which can be
// used to construct Shell commands and read the response.
#include <Process.h>
#include <Console.h>

/////////////////
// Phant Stuff //
/////////////////
// URL to phant server (only change if you're not using data.sparkfun
String phantURL = "http://data.sparkfun.com/input/";
// Public key (the one you see in the URL):
String publicKey = "xRpV3GA0jVHMnoOnor2K";
// Private key, which only someone posting to the stream knows
String privateKey = "Zar2ERmej2TAMX2MXVG0";
// How many data fields are in your stream?
const int NUM_FIELDS = 8;
// What are the names of your fields?
String fieldName[NUM_FIELDS] = {"humidity", "tempf", "moisture1", "moisture2", "moisture3", "moisture4", "moisture5", "moisture6"};
// We'll use this array later to store our field data
String fieldData[NUM_FIELDS] = {"0"};

////////////////
// Pin Inputs //
////////////////
const uint8_t analog_pins[] = {A0,A1,A2,A3,A4,A5};
const uint8_t digital_pins[] = {3,4,5,6,7,8};
const int NUM_SENSORS = 1;
const int SENSOR_OFFSET = 2; // Equals index of moisture1

////////
// DHT11
////////
#include "DHT.h"

#define DHTPIN 2     // what digital pin we're connected to

// Uncomment whatever type you're using!
#define DHTTYPE DHT11   // DHT 11
//#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
//#define DHTTYPE DHT21   // DHT 21 (AM2301)

// Connect pin 1 (on the left) of the sensor to +5V
// NOTE: If using a board with 3.3V logic like an Arduino Due connect pin 1
// to 3.3V instead of 5V!
// Connect pin 2 of the sensor to whatever your DHTPIN is
// Connect pin 4 (on the right) of the sensor to GROUND
// Connect a 10K resistor from pin 2 (data) to pin 1 (power) of the sensor

// Initialize DHT sensor.
// Note that older versions of this library took an optional third parameter to
// tweak the timings for faster processors.  This parameter is no longer needed
// as the current DHT reading algorithm adjusts itself to work on faster procs.
DHT dht(DHTPIN, DHTTYPE);

void setup() 
{
  Bridge.begin();
  Console.begin();
  Console.buffer(64);

  // Setup Output Pins:
  for (int i = 0; i < NUM_SENSORS; i++) {
    pinMode(digital_pins[i], OUTPUT);
    digitalWrite(i, LOW);
  }
  
  dht.begin();
  
  for (int i = 0; i < 10; i++) {
    Console.println("=========== Ready to Stream ===========");
    Console.flush();
    delay(500);
  }
}

void loop()
{  
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  fieldData[0] = String(h);
  // Read temperature as Fahrenheit (isFahrenheit = true)
  float t = dht.readTemperature(true);
  fieldData[1] = String(t);

  for (int i = 0; i < NUM_SENSORS; i++) {
    // Enable power to sensor
    digitalWrite(i, HIGH);
    // Give time to settle
    delay(100);
    
    int val = analogRead(i);
    fieldData[SENSOR_OFFSET + i] = String(val);
    
    // Disable power to sensor
    digitalWrite(i, LOW);
  }

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t)) {
    Console.println("Failed to read from DHT sensor!");  
    Console.flush();
    delay(500);
    return;
  } else {
    postData();
  }

  Console.println("Sleeping");
  Console.flush();
    
  // Wait a few seconds between measurements.
  delay(30000);
}

void postData()
{
  Process phant; // Used to send command to Shell, and view response
  String curlCmd; // Where we'll put our curl command
  String curlData[NUM_FIELDS]; // temp variables to store curl data

  // Construct curl data fields
  // Should look like: --data "fieldName=fieldData"
  for (int i=0; i<NUM_FIELDS; i++)
  {
    curlData[i] = "--data \"" + fieldName[i] + "=" + fieldData[i] + "\" ";
  }

  // Construct the curl command:
  curlCmd = "curl ";
  curlCmd += "--header "; // Put our private key in the header.
  curlCmd += "\"Phant-Private-Key: "; // Tells our server the key is coming
  curlCmd += privateKey; 
  curlCmd += "\" "; // Enclose the entire header with quotes.
  for (int i=0; i<NUM_FIELDS; i++)
    curlCmd += curlData[i]; // Add our data fields to the command
  curlCmd += phantURL + publicKey; // Add the server URL, including public key

  // Send the curl command:
  Console.print("Sending command: ");
  Console.println(curlCmd); // Print command for debug
  phant.runShellCommand(curlCmd); // Send command through Shell

  // Read out the response:
  Console.print("Response: ");
  // Use the phant process to read in any response from Linux:
  while (phant.available())
  {
    char c = phant.read();
    Console.write(c);
  }
  Console.println("");
}

