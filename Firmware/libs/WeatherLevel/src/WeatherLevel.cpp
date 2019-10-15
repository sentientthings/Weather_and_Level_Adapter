/*
  WeatherLevel.h - Weather and Level adapter library
  Copyright (c) 2018 Sentient Things, Inc.  All right reserved.
*/



// include this library's description file
#include "WeatherLevel.h"

#include <math.h>


// Constructor /////////////////////////////////////////////////////////////////
// Function that handles the creation and setup of instances

/*WeatherLevel::WeatherLevel()
{
  // initialize this instance's variables


  // do whatever is required to initialize the library

}
*/
// Public Methods //////////////////////////////////////////////////////////////
// Functions available in Wiring sketches, this library, and other libraries
void WeatherLevel::begin(void)
{
//  pinMode(AnemometerPin, INPUT_PULLUP);
  AnemoneterPeriodTotal = 0;
  AnemoneterPeriodReadingCount = 0;
  GustPeriod = UINT_MAX;  //  The shortest period (and therefore fastest gust) observed
  lastAnemoneterEvent = 0;
//  attachInterrupt(AnemometerPin, handleAnemometerEvent, FALLING);

  barom.begin();
  barom.setModeBarometer();
  barom.setOversampleRate(7);
  barom.enableEventFlags();

  am2315.begin();

  ds18b20.begin();
  ds18b20.setResolution(11);
  ds18b20.requestTemperatures();

  memset(serial1Buf, NULL, sizeof serial1Buf);
}

float  WeatherLevel::getAndResetAnemometerMPH(float * gustMPH)
{
    if(AnemoneterPeriodReadingCount == 0)
    {
        *gustMPH = 0.0;
        return 0;
    }
    // Nonintuitive math:  We've collected the sum of the observed periods between pulses, and the number of observations.
    // Now, we calculate the average period (sum / number of readings), take the inverse and muliple by 1000 to give frequency, and then mulitply by our scale to get MPH.
    // The math below is transformed to maximize accuracy by doing all muliplications BEFORE dividing.
    float result = AnemometerScaleMPH * 1000.0 * float(AnemoneterPeriodReadingCount) / float(AnemoneterPeriodTotal);
    AnemoneterPeriodTotal = 0;
    AnemoneterPeriodReadingCount = 0;
    *gustMPH = AnemometerScaleMPH  * 1000.0 / float(GustPeriod);
    GustPeriod = UINT_MAX;
    return result;
}


float WeatherLevel::getAndResetRainInches()
{
    float result = RainScaleInches * float(rainEventCount);
    rainEventCount = 0;
    return result;
}
/// Wind Vane
void WeatherLevel::captureWindVane() {
    // Read the wind vane, and update the running average of the two components of the vector
    unsigned int windVaneRaw = analogRead(WindVanePin);
//Serial.println(windVaneRaw);
    float windVaneRadians = lookupRadiansFromRaw(windVaneRaw);
//Serial.println(windVaneRadians);
    if(windVaneRadians > 0 && windVaneRadians < 6.14159)
    {
        windVaneCosTotal += cos(windVaneRadians);
        windVaneSinTotal += sin(windVaneRadians);
        windVaneReadingCount++;
    }
    return;
}

float WeatherLevel::getAndResetWindVaneDegrees()
{
    if(windVaneReadingCount == 0) {
        return 0;
    }
    float avgCos = windVaneCosTotal/float(windVaneReadingCount);
    float avgSin = windVaneSinTotal/float(windVaneReadingCount);
    float result = atan(avgSin/avgCos) * 180.0 / 3.14159;
    windVaneCosTotal = 0.0;
    windVaneSinTotal = 0.0;
    windVaneReadingCount = 0;
    // atan can only tell where the angle is within 180 degrees.  Need to look at cos to tell which half of circle we're in
    if(avgCos < 0) result += 180.0;
    // atan will return negative angles in the NW quadrant -- push those into positive space.
    if(result < 0) result += 360.0;

   return result;
}

float WeatherLevel::lookupRadiansFromRaw(unsigned int analogRaw)
{
//Serial.println(analogRaw);
    // The mechanism for reading the weathervane isn't arbitrary, but effectively, we just need to look up which of the 16 positions we're in.
    if(analogRaw >= 2200 && analogRaw < 2400) return (3.14);//South
    if(analogRaw >= 2100 && analogRaw < 2200) return (3.53);//SSW
    if(analogRaw >= 3200 && analogRaw < 3299) return (3.93);//SW
    if(analogRaw >= 3100 && analogRaw < 3200) return (4.32);//WSW
    if(analogRaw >= 3890 && analogRaw < 3999) return (4.71);//West
    if(analogRaw >= 3700 && analogRaw < 3780) return (5.11);//WNW
    if(analogRaw >= 3780 && analogRaw < 3890) return (5.50);//NW
    if(analogRaw >= 3400 && analogRaw < 3500) return (5.89);//NNW
    if(analogRaw >= 3570 && analogRaw < 3700) return (0.00);//North
    if(analogRaw >= 2600 && analogRaw < 2700) return (0.39);//NNE
    if(analogRaw >= 2750 && analogRaw < 2850) return (0.79);//NE
    if(analogRaw >= 1510 && analogRaw < 1580) return (1.18);//ENE
    if(analogRaw >= 1580 && analogRaw < 1650) return (1.57);//East
    if(analogRaw >= 1470 && analogRaw < 1510) return (1.96);//ESE
    if(analogRaw >= 1900 && analogRaw < 2000) return (2.36);//SE
    if(analogRaw >= 1700 && analogRaw < 1750) return (2.74);//SSE
    if(analogRaw > 4000) return(-1); // Open circuit?  Probably means the sensor is not connected
   // Particle.publish("error", String::format("Got %d from Windvane.",analogRaw), 60 , PRIVATE);
    return -1;
}

/// end Wind vane

void WeatherLevel::captureTempHumidityPressure() {
  // Read the humidity and pressure sensors, and update the running average
  // The running (mean) average is maintained by keeping a running sum of the observations,
  // and a count of the number of observations

  // Measure Relative Humidity and temperature from the AM2315
  float humidityRH, tempC, tempF;
  bool validTH = am2315.readTemperatureAndHumidity(tempC, humidityRH);

  uint16_t tempKx10 = uint16_t(tempC*10)+2732;
  airTempKMedian.add(tempKx10);

  relativeHumidtyMedian.add(humidityRH);

if (validTH){
    //If the result is reasonable, add it to the running mean
    if(humidityRH > 0 && humidityRH < 105) // It's theoretically possible to get supersaturation humidity levels over 100%
    {
        // Add the observation to the running sum, and increment the number of observations
        humidityRHTotal += humidityRH;
        humidityRHReadingCount++;
    }


    tempF = (tempC * 9.0) / 5.0 + 32.0;
    //If the result is reasonable, add it to the running mean
    if(tempF > -50 && tempF < 150)
    {
        // Add the observation to the running sum, and increment the number of observations
        tempFTotal += tempF;
        tempFReadingCount++;
    }
  }
  //Measure Pressure from the MPL3115A2
  float pressurePascals = barom.readPressure();

  //If the result is reasonable, add it to the running mean
  // What's reasonable? http://findanswers.noaa.gov/noaa.answers/consumer/kbdetail.asp?kbid=544
  if(pressurePascals > 80000 && pressurePascals < 110000)
  {
      // Add the observation to the running sum, and increment the number of observations
      pressurePascalsTotal += pressurePascals;
      pressurePascalsReadingCount++;
  }

  //Measure water temperature from the DS18B20
  float waterTempC = ds18b20.getCRCTempC();
  float waterTempF = (waterTempC * 9.0) / 5.0 + 32.0;
  if(waterTempF > -50 && waterTempF < 150)
  {
      // Add the observation to the running sum, and increment the number of observations
      waterTempFTotal += waterTempF;
      waterTempFReadingCount++;
  }
delay(2);
ds18b20.requestTemperatures();
  return;
}

void WeatherLevel::captureAirTempHumid() {
  // Read the humidity and pressure sensors, and update the running average
  // The running (mean) average is maintained by keeping a running sum of the observations,
  // and a count of the number of observations

  // Measure Relative Humidity and temperature from the AM2315
  float humidityRH, tempC, tempF;
  bool validTH = am2315.readTemperatureAndHumidity(tempC, humidityRH);

  uint16_t tempKx10 = uint16_t(tempC*10.0+2732.0);
  airTempKMedian.add(tempKx10);

  relativeHumidtyMedian.add(humidityRH);

  if (validTH){
      //If the result is reasonable, add it to the running mean
      if(humidityRH > 0 && humidityRH < 105) // It's theoretically possible to get supersaturation humidity levels over 100%
      {
          // Add the observation to the running sum, and increment the number of observations
          humidityRHTotal += humidityRH;
          humidityRHReadingCount++;
      }


      tempF = (tempC * 9.0) / 5.0 + 32.0;
      //If the result is reasonable, add it to the running mean
      if(tempF > -50 && tempF < 150)
      {
          // Add the observation to the running sum, and increment the number of observations
          tempFTotal += tempF;
          tempFReadingCount++;
      }
    }
}

void WeatherLevel::captureWaterTemp() {
  //Measure water temperature from the DS18B20
  float waterTempC = ds18b20.getCRCTempC();
  float waterTempF = (waterTempC * 9.0) / 5.0 + 32.0;
  if(waterTempF > -50 && waterTempF < 150)
  {
      // Add the observation to the running sum, and increment the number of observations
      waterTempFTotal += waterTempF;
      waterTempFReadingCount++;
  }
  delay(2);
  ds18b20.requestTemperatures();
}

void WeatherLevel::capturePressure() {
  //Measure Pressure from the MPL3115A2
  float pressurePascals = barom.readPressure();

  //If the result is reasonable, add it to the running mean
  // What's reasonable? http://findanswers.noaa.gov/noaa.answers/consumer/kbdetail.asp?kbid=544
  if(pressurePascals > 80000 && pressurePascals < 110000)
  {
      // Add the observation to the running sum, and increment the number of observations
      pressurePascalsTotal += pressurePascals;
      pressurePascalsReadingCount++;
  }
}

float WeatherLevel::getAndResetTempF()
{
    if(tempFReadingCount == 0) {
        return 0;
    }
    float result = tempFTotal/float(tempFReadingCount);
    tempFTotal = 0.0;
    tempFReadingCount = 0;
    return result;
}

float WeatherLevel::getAndResetHumidityRH()
{
    if(humidityRHReadingCount == 0) {
        return 0;
    }
    float result = humidityRHTotal/float(humidityRHReadingCount);
    humidityRHTotal = 0.0;
    humidityRHReadingCount = 0;
    return result;
}


float WeatherLevel::getAndResetPressurePascals()
{
    if(pressurePascalsReadingCount == 0) {
        return 0;
    }
    float result = pressurePascalsTotal/float(pressurePascalsReadingCount);
    pressurePascalsTotal = 0.0;
    pressurePascalsReadingCount = 0;
    return result;
}

float WeatherLevel::getAndResetWaterTempF()
{
    if(waterTempFReadingCount == 0) {
        return 0;
    }
    float result = waterTempFTotal/float(waterTempFReadingCount);
    waterTempFTotal = 0.0;
    waterTempFReadingCount = 0;
    return result;
}

float WeatherLevel::getWaterTempC(void)
{
  float tempC = ds18b20.getCRCTempC();
  ds18b20.requestTemperatures();
  return tempC;

}

int16_t WeatherLevel::getWaterTempRAW(void)
{
  int16_t tempC = ds18b20.getCRCTempRAW();
  ds18b20.requestTemperatures();
  return tempC;

}

void WeatherLevel::parseMaxbotixToBuffer()
{
    char c = Serial1.read();
// Serial.print(c);


    // Check for start of range reading RxxxCR or RxxxxCR
    // Can be mm, cm, or inches.
    if (c == 'R')
    {
        readingRange = true;
        rangeBegin = millis();
        serial1BufferIndex = 0;
        memset(serial1Buf, 0x00, 6);
        return;
    }

    // True if R has been received ..
    if (readingRange)
    {
        // valid if last R was less than 500ms ago
        if (millis()-rangeBegin<=500)
        {
            // To test valid range digits from 0 to 9
            uint8_t cnum = c -'0';
            // True if this is the end of the range message CR
            if (c=='\r')
            {
                // and the number of digits is 3 or 4
                if (serial1BufferIndex==3 || serial1BufferIndex==4)
                {
  // Serial.println(String(serial1Buf));
                    String range = String(serial1Buf);
                    int rangeInt = range.toInt();
                    maxbotixMedian.add((uint16_t) rangeInt);
                    readingRange = false;
                    serial1BufferIndex = 0;
                    memset(serial1Buf, 0x00, 6);
                    return;
                }
                // if not then there is an error
                else
                {
                   readingRange = false;
                   serial1BufferIndex = 0;
                   memset(serial1Buf, 0x00, 6);
                   return;
                }
            }
            // if the received char is 0 to 9 add it to the buffer
            else if (cnum >= 0 && cnum <=9)
            {
                serial1Buf[serial1BufferIndex] = c;
                serial1BufferIndex++;
                return;
            }
            // if neither then there is an error
            else
            {
                readingRange = false;
                serial1BufferIndex = 0;
                memset(serial1Buf, 0x00, 6);
                return;
            }
        }
        else
        // readings have timed out so reset
        {
            readingRange = false;
            serial1BufferIndex = 0;
            memset(serial1Buf, 0x00, 6);
            return;
        }
    }
    else
    {
        return;
    }
}


uint16_t WeatherLevel::getRangeMedian()
{
  uint16_t rangeMedian = maxbotixMedian.getMedian();
  return rangeMedian;
}

uint16_t WeatherLevel::getAirTempKMedian()
{
  uint16_t airKMedian = airTempKMedian.getMedian();
  return airKMedian;
}

uint16_t WeatherLevel::getRHMedian()
{
  uint16_t RHMedian = relativeHumidtyMedian.getMedian();
  return RHMedian;
}

// bool WeatherLevel::getAirTempAndHumidRAW(int16_t &tRAW, uint16_t &hRAW)
// {
// if(am2315.getTemperatureAndHumidityRAW(tRAW, hRAW))
//   {
//     return true;
//   }
//   else
//   {
//     return false;
//   }
// //  am2315.requestTemperatureAndHumidity();
// }

float WeatherLevel::readPressure()
{
  return barom.readPressure();
}
/*
//Weather
uint32_t unixTime;
uint8_t windDegrees; // 1 degree resolution is plenty
uint16_t wind_metersph; //meters per hour
uint8_t humid; //percent
uint16_t airTempKx10; // Temperature in deciKelvin
uint16_t rainmm; // millimeters
float barometerkPa; // Could fit into smaller type if needed
uint16_t gust_metersph; //meters per hour
// Water
uint16_t rangemm; //Range in millimeters
uint16_t waterTempKx10; // Temperature in deciKelvin
*/
void WeatherLevel::getAndResetAllSensors()
{
  uint32_t timeRTC = rtc.rtcNow();
  sensorReadings.unixTime = timeRTC;
  float gustMPH;
  float windMPH = getAndResetAnemometerMPH(&gustMPH);
  sensorReadings.wind_metersph = (uint16_t) ceil(windMPH * 1609.34);
  float rainInches = getAndResetRainInches();
  sensorReadings.rainmmx1000 = (uint16_t) ceil(rainInches * 25400);
  float windDegrees = getAndResetWindVaneDegrees();
  sensorReadings.windDegrees = (uint16_t) ceil(windDegrees);
  float airTempF = getAndResetTempF();
  sensorReadings.airTempKx10 = (uint16_t) ceil((airTempF-32.0)*50.0/9.0 + 2731.5);
  // sensorReadings.airTempKx10 = airTempKMedian.getMedian();
  // float humidityRH = getAndResetHumidityRH();
  uint16_t humidityRH = relativeHumidtyMedian.getMedian();
  sensorReadings.humid =(uint8_t) ceil(humidityRH);
  float pressure = getAndResetPressurePascals();
  sensorReadings.barometerhPa = pressure/10.0;
  float waterTempF = getAndResetWaterTempF();
  sensorReadings.waterTempKx10 = (uint16_t) ceil((waterTempF-32.0)*50.0/9.0 + 2731.5);
  uint16_t range;
  // Could be in in.=i or cm=c or mm=m depending on the sensor
  switch (config.maxbotixType)
  {
    case 'i':
      range = (uint16_t)ceil((float)getRangeMedian()*25.4);
      break;
    case 'c':
      range = getRangeMedian()*10;
      break;
    case 'm':
      range = getRangeMedian();
      break;
    default:
      range = getRangeMedian();
  }
  sensorReadings.rangemm = range;
}

// Convert sensorData to CSV String in US units
String WeatherLevel::sensorReadingsToCsvUS()
{
  String csvData =
//  String(Time.format(sensorReadings.unixTime, TIME_FORMAT_ISO8601_FULL))+
  String(sensorReadings.unixTime)+
  ","+
  String(sensorReadings.windDegrees)+
  ","+
  minimiseNumericString(String::format("%.1f",(float)sensorReadings.wind_metersph/1609.34),1)+
  ","+
  String(sensorReadings.humid)+
  ","+
  minimiseNumericString(String::format("%.1f",(((float)sensorReadings.airTempKx10-2731.5)*9.0/50.0+32.0)),1)+
  ","+
  minimiseNumericString(String::format("%.3f",((float)sensorReadings.rainmmx1000/25400)),3)+
  ","+
  minimiseNumericString(String::format("%.2f",(float)sensorReadings.barometerhPa/338.6389),2)+
// Not enough fields in thingspeak channel for gust
//  ","+
//  minimiseNumericString(String::format("%.1f",(Float)(sensorReadings.gust_metersph/1609.34)),1)+
  ","+
  minimiseNumericString(String::format("%.2f",(((float)sensorReadings.rangeReferencemm-(float)sensorReadings.rangemm)/304.8)),2)+ // 25.4*12=304.8
  ","+
  minimiseNumericString(String::format("%.1f",(((float)sensorReadings.waterTempKx10-2731.5)*9/50.0+32.0)),1)+
  ",,,,"+
  String(sensorReadings.rangeReferencemm)
  ;
//add status field = range reference
  return csvData;
}


// Convert sensorData to CSV String in US units
String WeatherLevel::sensorReadingsToCsvUS(sensorReadings_t readings)
{
  String csvData =
//  String(Time.format(readings.unixTime, TIME_FORMAT_ISO8601_FULL))+
  String(readings.unixTime)+
  ","+
  String(readings.windDegrees)+
  ","+
  minimiseNumericString(String::format("%.1f",(float)readings.wind_metersph/1609.34),1)+
  ","+
  String(readings.humid)+
  ","+
  minimiseNumericString(String::format("%.1f",(((float)readings.airTempKx10-2731.5)*9.0/50.0+32.0)),1)+
  ","+
  minimiseNumericString(String::format("%.3f",((float)readings.rainmmx1000/25400)),3)+
  ","+
  minimiseNumericString(String::format("%.2f",(float)readings.barometerhPa/338.6389),2)+
// Not enough fields in thingspeak for this
//  ","+
//  minimiseNumericString(String::format("%.1f",(Float)(readings.gust_metersph/1609.34)),1)+
  ","+
  minimiseNumericString(String::format("%.2f",(((float)readings.rangeReferencemm-readings.rangemm)/304.8)),2)+
  ","+
  minimiseNumericString(String::format("%.1f",(((float)readings.waterTempKx10-2731.5)*9.0/50.0+32.0)),1)+
  ",,,,"+
  String(readings.rangeReferencemm)
  ;

  return csvData;
}

// Private Methods /////////////////////////////////////////////////////////////
// Functions only available to other functions in this library

//https://stackoverflow.com/questions/277772/avoid-trailing-zeroes-in-printf
String WeatherLevel::minimiseNumericString(String ss, int n) {
    int str_len = ss.length() + 1;
    char s[str_len];
    ss.toCharArray(s, str_len);

//Serial.println(s);
    char *p;
    int count;

    p = strchr (s,'.');         // Find decimal point, if any.
    if (p != NULL) {
        count = n;              // Adjust for more or less decimals.
        while (count >= 0) {    // Maximum decimals allowed.
             count--;
             if (*p == '\0')    // If there's less than desired.
                 break;
             p++;               // Next character.
        }

        *p-- = '\0';            // Truncate string.
        while (*p == '0')       // Remove trailing zeros.
            *p-- = '\0';

        if (*p == '.') {        // If all decimals were zeros, remove ".".
            *p = '\0';
        }
    }
    return String(s);
}
