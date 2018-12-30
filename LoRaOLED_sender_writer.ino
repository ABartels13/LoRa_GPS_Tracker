#include <Wire.h>  // Only needed for Arduino 1.6.5 and earlier
#include "SSD1306.h" // alias for `#include "SSD1306Wire.h"`

#include <SPI.h>
#include <LoRa.h>
// #include "SSD1306.h"
#include<Arduino.h>


#include <TinyGPS++.h> 
#include <HardwareSerial.h>

#define SS 18
#define RST 14
#define DI0 26
#define NODE_ID 0

TinyGPSPlus gps;                            
HardwareSerial MySerial(1);                 

#define BAND 915000000.00 //BAND 434500000.00 -> 915000000.00

#define spreadingFactor 10 //9 -> 12
#define SignalBandwidth 125E3//62.5E3 //62.5
// #define SignalBandwidth 31.25E3
#define preambleLength 8
#define codingRateDenominator 8

struct payload{
  char node_id;
  double latitude;
  double longitude;
  int time_sec;
  char chk_sum;
};

int counter = 0;

static void smartDelay(unsigned long ms)                
{
  unsigned long start = millis();
  do
  {
    while (MySerial.available())
      gps.encode(MySerial.read());
  } while (millis() - start < ms);
}

void setup() {
  //GPS Setup
  MySerial.begin(9600, SERIAL_8N1, 12, 15);   //17-TX 18-RX
  
  pinMode(25,OUTPUT); //Send success, LED will bright 1 second 
  Serial.begin(115200);
  while (!Serial); //If just the the basic function, must connect to a computer

  SPI.begin(5,19,27,18);
  LoRa.setPins(SS,RST,DI0);
  Serial.println("LoRa Sender");
  if (!LoRa.begin(BAND)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
  
  Serial.print("LoRa Spreading Factor: ");
  Serial.println(spreadingFactor);
  LoRa.setSpreadingFactor(spreadingFactor);
  
  Serial.print("LoRa Signal Bandwidth: ");
  Serial.println(SignalBandwidth);
  LoRa.setSignalBandwidth(SignalBandwidth);

  LoRa.setCodingRate4(codingRateDenominator);

  LoRa.setPreambleLength(preambleLength);
  
  Serial.println("LoRa Initial OK!");
}

void loop() {

  payload packet;
  packet.node_id = NODE_ID;
  packet.latitude = gps.location.lat();
  packet.longitude = gps.location.lng();
  packet.time_sec = gps.time.second();
  packet.chk_sum = packet.node_id + packet.latitude + packet.longitude +  packet.time_sec;
  
  //Print to serial
  Serial.print(NODE_ID); //node_id
  Serial.print(",");
  Serial.print(gps.location.lat(), 5);
  Serial.print(",");
  Serial.print(gps.location.lng(), 5);
  Serial.print(",");
  Serial.print(gps.time.second());
  
  // send packet
  /*
  LoRa.beginPacket();
  LoRa.print(NODE_ID);
  LoRa.print(",");
  LoRa.print(gps.location.lat(), 5);
  LoRa.print(",");
  LoRa.print(gps.location.lng(), 5);
  LoRa.print(",");
  LoRa.print(gps.time.second());
  LoRa.endPacket();
  */

  //Sending Side
  char b[sizeof(packet)];
  memcpy(b, &packet, sizeof(packet));
    
  LoRa.beginPacket();
  LoRa.write((const uint8_t*)b, sizeof(b));
  LoRa.endPacket();
  
  /*counter++;
  digitalWrite(25, HIGH); // turn the LED on (HIGH is the voltage level)
  delay(1000); // wait for a second
  digitalWrite(25, LOW); // turn the LED off by making the voltage LOW
  delay(1000); // wait for a second
  */
  int randDelay = random(0,10000);
  smartDelay(10000+randDelay);
  if (millis() > 5000 && gps.charsProcessed() < 10)
    Serial.println(F("No GPS data received: check wiring"));
  
// delay(3000);
}
