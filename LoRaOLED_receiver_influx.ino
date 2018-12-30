#include <Wire.h>  // Only needed for Arduino 1.6.5 and earlier
#include <HTTPClient.h>


#include <SPI.h>
#include <LoRa.h>


#define SS 18
#define RST 14
#define DI0 26
#define BUFFER_SIZE 32

// LoRa Settings 
#define BAND 915000000.00
#define spreadingFactor 10
#define SignalBandwidth 125E3


#define codingRateDenominator 8


const char* ssid = "Telstra19C8";
const char* password = "4203745208";

const char* host = "68.183.183.89";

const uint16_t port = 8086;
const uint16_t numChars = 32;

struct payload{
  char node_id;
  double latitude;
  double longitude;
  int time_sec;
  char chk_sum;
};

payload packet; //Re-make the struct 
void setup() {

  Serial.begin(115200);
 
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("...");
  }
 
  Serial.print("WiFi connected with IP: ");
  Serial.println(WiFi.localIP());

  
  Serial.println("LoRa Receiver");
  SPI.begin(5,19,27,18);
  LoRa.setPins(SS,RST,DI0);


  if (!LoRa.begin(BAND)) {
    while (1);
  }
  Serial.println("LoRa Initial OK!");
  
  Serial.print("LoRa Frequency: ");
  Serial.println(BAND);
  
  Serial.print("LoRa Spreading Factor: ");
  Serial.println(spreadingFactor);
  LoRa.setSpreadingFactor(spreadingFactor);
  
  Serial.print("LoRa Signal Bandwidth: ");
  Serial.println(SignalBandwidth);
  LoRa.setSignalBandwidth(SignalBandwidth);

  LoRa.setCodingRate4(codingRateDenominator);
  
}

void loop() {
  uint8_t  _rx_buffer[sizeof(packet)];
  // try to parse packet
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    // received packet
    
    // read packet into buffer
    while (LoRa.available()) {
      LoRa.readBytes(_rx_buffer, sizeof(packet));
    }

    //Convert raw byte packet to payload struct
    char data_char[sizeof(packet)];
    memcpy(&packet, _rx_buffer, sizeof(packet));

    //Regenerate the checksum to check if we have a valid payload
    char test_chk_sum = packet.node_id + packet.latitude + packet.longitude + packet.time_sec;
    
    if(test_chk_sum == packet.chk_sum){
      //Valid checksum
      //Construct payload data
      //Takes the form of "location,node_id=0, gate_id=0 lat=-31,long=116,SNR=100.0"
      String data = String("location,node_id=");
      data += String((int)packet.node_id);
      data += String(",gate_id=0 lat=");
      data += String(packet.latitude, 5);
      data += String(",long=");
      data += String(packet.longitude, 5);
      data += String(",SNR=");
      data += String(double(LoRa.packetSnr()),3);
  
      //Initialise the http client and POST data
      HTTPClient http;   
      http.begin("http://68.183.183.89:8086/write?db=mydb&u=esp32&p=lora_tracker");  //Specify destination for HTTP request
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");             //Specify content-type header
      int httpResponseCode = http.POST(data);   //Send the actual POST request
 
       if(httpResponseCode>0){
        String response = http.getString();                       //Get the response to the request
        Serial.println(httpResponseCode);   //Print return code
        Serial.println(response);           //Print request answer
       }else{
        Serial.print("Error on sending POST: ");
        Serial.println(httpResponseCode);
       }
 
       http.end();  //Free resources
    }
 
    delay(50);
    
  }
}
