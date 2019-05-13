/*
  WeatherLevel.h - Weather and Level adapter library
  Copyright (c) 2018 Sentient Things, Inc.  All right reserved.
  Based on work by Rob Purser, Mathworks, Inc.
*/

// ensure this library description is only included once
#ifndef WeatherLevel_h
#define WeatherLevel_h

// include core Particle library
#include "application.h"



// include description files for other libraries used (if any)
#include <OneWire.h>
#include <DS18B20Minimal.h>
#include <Adafruit_AM2315.h>
#include <SparkFun_MPL3115A2.h>
#include <MCP7941x.h>
#include <IoTNodeWeatherLevelGlobals.h>


// // Globals defined here rather than a separate .h so that there is only one file
// // to reference in the Particle program api.
//
//
// typedef struct // units chosen for data size and readability
// {
//     //Weather
//     uint32_t unixTime; //system_tick_t (uint32_t)
//     uint16_t windDegrees; // 1 degree resolution is plenty
//     uint16_t wind_metersph; //meters per hour
//     uint8_t humid; //percent
//     uint16_t airTempKx10; // Temperature in deciKelvin
//     uint16_t rainmm; // millimeters
//     float barometerhPa; // Could fit into smaller type if needed
//     uint16_t gust_metersph; //meters per hour
//     // Water
//     int16_t rangemm; //Range in millimeters
//     uint16_t rangeReferencemm; // Range datum i.e. could be mllw range for tide
//     uint16_t waterTempKx10; // Temperature in deciKelvin
//
// }sensorReadings_t;
// extern sensorReadings_t sensorReadings;
//
// //struct to save created TS channel Id and keys and to check "first run"
// typedef struct
// {
//   int channelId;
//   int testCheck;
//   char writeKey[17];
//   char readKey[17];
//   char maxbotixType; // i = inches, m = mm, c = cm
//   char unitType; // U = USA, I = international
//   int firmwareVersion;
//   // int maxReadings;//=dataRing size
//   int particleTimeout;
//   uint16_t rangeReferencemm;
//   float latitude;
//   float longitude;
// }config_t;
// extern config_t config;
//
// // These variables change as the device logs
// typedef struct
// {
//     // Indices for the FRAM ring buffer.  Saved in FRAM for persistence
//     // during power cycling and battery loss.
//     unsigned long ringStart;
//     unsigned long ringEnd;
// }status_t;
// extern status_t status;
//
// //End of Globals

#include <RunningMedian16Bit.h>

// Updated for Oct 2018 v2 Weather and Level board
#define ONE_WIRE_BUS N_D4//N_D6//DAC

// library interface description
class WeatherLevel
{
  // user-accessible "public" interface
  public:
    WeatherLevel() : oneWire(ONE_WIRE_BUS), ds18b20(&oneWire), maxbotixMedian(650), airTempKMedian(30), relativeHumidtyMedian(30), rtc()
    // WeatherLevel() : maxbotixMedian(650), airTempKMedian(30), relativeHumidtyMedian(30), iotnode()
    {
      pinMode(AnemometerPin, INPUT_PULLUP);
      attachInterrupt(AnemometerPin, &WeatherLevel::handleAnemometerEvent, this, FALLING);

      pinMode(RainPin, INPUT_PULLUP);
      attachInterrupt(RainPin, &WeatherLevel::handleRainEvent, this, FALLING);
    }
    void handleAnemometerEvent() {
      // Activated by the magnet in the anemometer (2 ticks per rotation), attached to input D3
       unsigned int timeAnemometerEvent = millis(); // grab current time

      //If there's never been an event before (first time through), then just capture it
      if(lastAnemoneterEvent != 0) {
          // Calculate time since last event
          unsigned int period = timeAnemometerEvent - lastAnemoneterEvent;
          // ignore switch-bounce glitches less than 10mS after initial edge (which implies a max windspeed of 149mph)
          if(period < 10) {
            return;
          }
          if(period < GustPeriod) {
              // If the period is the shortest (and therefore fastest windspeed) seen, capture it
              GustPeriod = period;
          }
          AnemoneterPeriodTotal += period;
          AnemoneterPeriodReadingCount++;
      }

      lastAnemoneterEvent = timeAnemometerEvent; // set up for next event
    }

    void handleRainEvent() {
      // Count rain gauge bucket tips as they occur
      // Activated by the magnet and reed switch in the rain gauge, attached to input D2
      unsigned int timeRainEvent = millis(); // grab current time

      // ignore switch-bounce glitches less than 10mS after initial edge
      if(timeRainEvent - lastRainEvent < 10) {
        return;
      }
      rainEventCount++; //Increase this minute's amount of rain
      lastRainEvent = timeRainEvent; // set up for next event
    }

    float getWaterTempC(void);
    int16_t getWaterTempRAW(void);
    void begin(void);
    // bool getAirTempAndHumidRAW(int16_t &tRAW, uint16_t &hRAW);
    float readPressure();

    float getAndResetAnemometerMPH(float * gustMPH);
    float getAndResetRainInches();

    void captureWindVane();
    void captureTempHumidityPressure();
    void captureAirTempHumid();
    void captureWaterTemp();
    void capturePressure();

    float getAndResetWindVaneDegrees();
    float getAndResetTempF();
    float getAndResetHumidityRH();
    float getAndResetPressurePascals();
    float getAndResetWaterTempF();
    void getAndResetAllSensors();

    void parseMaxbotixToBuffer();

    uint16_t getRangeMedian();
    uint16_t getAirTempKMedian();
    uint16_t getRHMedian();

    String sensorReadingsToCsvUS();
    String sensorReadingsToCsvUS(sensorReadings_t readings);

  // library-accessible "private" interface
  private:
    OneWire oneWire;
    DS18B20 ds18b20;
    Adafruit_AM2315 am2315;
    MPL3115A2 barom;
    RunningMedian maxbotixMedian;
    RunningMedian airTempKMedian;
    RunningMedian relativeHumidtyMedian;
    MCP7941x rtc;

    String minimiseNumericString(String ss, int n);


    // Put pin mappings to Particle microcontroller here for now as well as
    // required variables
    //int RainPin = D5;
    //int RainPin = N_D3;
    // Updated for Oct 2018 v2 Weather and Level board
    int RainPin = N_D2;
    volatile unsigned int rainEventCount;
    unsigned int lastRainEvent;
    float RainScaleInches = 0.011; // Each pulse is .011 inches of rain

    //const int AnemometerPin = D4;
    //const int AnemometerPin = N_D2;
    // Updated for Oct 2018 v2 Weather and Level board
    const int AnemometerPin = N_D1;
    float AnemometerScaleMPH = 1.492; // Windspeed if we got a pulse every second (i.e. 1Hz)
    volatile unsigned int AnemoneterPeriodTotal = 0;
    volatile unsigned int AnemoneterPeriodReadingCount = 0;
    volatile unsigned int GustPeriod = UINT_MAX;
    unsigned int lastAnemoneterEvent = 0;

    //int WindVanePin = A0;
    int WindVanePin = N_A2;
    float windVaneCosTotal = 0.0;
    float windVaneSinTotal = 0.0;
    unsigned int windVaneReadingCount = 0;

    float humidityRHTotal = 0.0;
    unsigned int humidityRHReadingCount = 0;
    float tempFTotal = 0.0;
    unsigned int tempFReadingCount = 0;
    float pressurePascalsTotal = 0.0;
    unsigned int pressurePascalsReadingCount = 0;
    float waterTempFTotal = 0.0;
    unsigned int waterTempFReadingCount = 0;

    // float lightLux = 0.0;
    // unsigned int lightLuxCount = 0;
    ///
    float lookupRadiansFromRaw(unsigned int analogRaw);

    char serial1Buf[6];
    uint8_t serial1BufferIndex = 0;
    bool readingRange = false;
    uint32_t rangeBegin;
};

#endif
