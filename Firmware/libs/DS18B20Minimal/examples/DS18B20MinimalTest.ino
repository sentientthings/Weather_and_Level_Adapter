// This #include statement was automatically added by the Particle IDE.
#include <OneWire.h>

// This #include statement was automatically added by the Particle IDE.
#include "DS18B20.h"

// Change this pin to the one your D18B20 is connected too
#define ONE_WIRE_BUS DAC

OneWire oneWire(ONE_WIRE_BUS);
DS18B20 sensor(&oneWire);

Timer timer(1000, printTemperature);

void setup(void)
{
  Serial.begin(115200);
  Serial.println(__FILE__);
  Serial.print("DS18B20 Library version: ");
  Serial.println(DS18B20_LIB_VERSION);

  sensor.begin();
  sensor.setResolution(11);
  timer.start();
}


void loop(void)
{
//sensor.requestTemperatures();
//Serial.println("waiting");
//delay(1200);
//  while (!sensor.isConversionComplete());  // wait until sensor is ready


//  Serial.println(sensor.getCRC(),HEX);

//  Serial.println(sensor.calCRC(),HEX);

//  Serial.print("Temp: ");
//  Serial.println(sensor.getCRCTempC());
}

void printTemperature()
{
    if (sensor.isConversionComplete())
    {
    Serial.print("Temp: ");
    Serial.println(sensor.getCRCTempC());
    sensor.requestTemperatures();
    }

}
