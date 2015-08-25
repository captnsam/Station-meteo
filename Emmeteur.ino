
/*.............................................................
Sending Multiple Variables Using VirtualWire. Transmitter
Author: Rodrigo Mompo Redoliww  http://controlrobotics.rodrigomompo.com
Modified by  deba168 from INDIA on 05/09/2014
..............................................................*/
#include <VirtualWire.h>
#include <LowPower.h>
#include <Wire.h>

#include "Adafruit_SI1145.h"
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP085_U.h>
#include "DHT.h"


#define DHTPIN 8     // what pin we're connected to
#define DHTTYPE DHT11   // DHT 11 
DHT dht(DHTPIN, DHTTYPE);
int ledPin = 13;
char Msg[35];// The string that we are going to send trought rf transmitter 
Adafruit_BMP085_Unified bmp = Adafruit_BMP085_Unified(10085);
Adafruit_SI1145 uv = Adafruit_SI1145();

void setup() 
{
  Serial.begin(9600);            //Port serial
  
  if(!bmp.begin())
  {
    /* There was a problem detecting the BMP085 ... check your connections */
    Serial.print("Ooops, no BMP085 detected ... Check your wiring or I2C ADDR!");
    while(1);
  }
  
  Serial.println("Adafruit SI1145 test");
  
  if (! uv.begin()) {
    Serial.println("Didn't find Si1145");
    while (1);
  }

  Serial.println("OK!");
 
  dht.begin();  // initialing the DHT sensor
  pinMode(ledPin,OUTPUT);
  // VirtualWire setup
  vw_setup(2000); // Bits per sec
  vw_set_tx_pin(12);// Set the Tx pin. Default is 12
}

//-------------------------------------------------------------//
 
void loop() 
{
  sensors_event_t event;
  bmp.getEvent(&event);
  
  int pressure = event.pressure;
  int humidity = dht.readHumidity();
  int temp = dht.readTemperature();
  int UVindex = uv.readUV();
  Serial.println(UVindex);
  
  sprintf(Msg, "%d,%d,%d,%d", humidity, temp, pressure, UVindex);
  
  digitalWrite(ledPin, HIGH); 
  
  delay(100); 
  vw_send((uint8_t *)Msg, strlen(Msg));
  vw_wait_tx(); // Wait until the whole message is gone
  digitalWrite(ledPin, LOW); 
  // put 5 mins sleep mode
  // As lowpower library support maximam 8s ,we use for loop to take longer (5mins) sleep
  // 5x60=300
  //300/4=75
  for(int i=0;i<75;i++)
  {
  LowPower.powerDown(SLEEP_4S, ADC_OFF, BOD_OFF);    // Instead of delay(4000); 
  //delay(4000);
  }
  
}
