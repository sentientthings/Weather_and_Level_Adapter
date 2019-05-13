/*
  MCP7941x.cpp - Arduino Library for using the MCP7941x IC.
  Ian Chilton <ian@ichilton.co.uk>
  November 2011
  Only currently supports 24hr clock and doesn't do any validation.

  Modified for use for the particle.io devices
  by Robert Mawrey
  June 2016
*/

//Removed to work with the particle.io devices
/*
#if (ARDUINO >= 100)
  #include <Arduino.h>
  #define WireSend(x) Wire.write(x)
  #define WireReceive() Wire.read()
#else
  #include <WProgram.h>
  #define WireSend(x) Wire.send(x)
  #define WireReceive(x) Wire.receive(x)
#endif
*/

// Added for the particle.io devices
#include "application.h"
  #define WireSend(x) Wire.write(x)
  #define WireReceive() Wire.read()

#include "MCP7941x.h"


// Constructor:
MCP7941x::MCP7941x()
{
  //  Wire.setSpeed(400000);
  // Initialize the I2C bus if not already enabled
  if (!Wire.isEnabled()) {
      Wire.begin();
  }
}


// Convert normal decimal numbers to binary coded decimal:
byte MCP7941x::decToBcd(byte val)
{
  return ( (val/10*16) + (val%10) );
}


// Convert binary coded decimal to normal decimal numbers:
byte MCP7941x::bcdToDec(byte val)
{
  return ( (val/16*10) + (val%16) );
}


// Function to read the mac address from the eeprom:
void MCP7941x::getMacAddress(byte *mac_address)
{
  Wire.beginTransmission(MCP7941x_EEPROM_I2C_ADDR);
  WireSend(MAC_LOCATION);
  Wire.endTransmission();

  Wire.requestFrom(MCP7941x_EEPROM_I2C_ADDR, 6);

  for( int i=0; i<6; i++ )
  {
    mac_address[i] = WireReceive();
  }
}


// Unlock the unique id area ready for writing:
void MCP7941x::unlockUniqueID()
{
  // Write 0x55 to the memory location 0x09 and stop:
  Wire.beginTransmission(MCP7941x_RTC_I2C_ADDR);
  WireSend(0x09);
  WireSend(0x55);
  Wire.endTransmission();

  // Write 0xAA to the memory location 0x09 and stop:
  Wire.beginTransmission(MCP7941x_RTC_I2C_ADDR);
  WireSend(0x09);
  WireSend(0xAA);
  Wire.endTransmission();
}


// Unlock the unique id area and write in the mac address:
void MCP7941x::writeMacAddress(byte *mac_address)
{
  Wire.beginTransmission(MCP7941x_EEPROM_I2C_ADDR);
  WireSend(0xF2);

  for( int i=0; i<6; i++ )
  {
    WireSend(mac_address[i]);
  }

  Wire.endTransmission();
}


// Set the date/time, set to 24hr and enable the clock:
// (assumes you're passing in valid numbers)
void MCP7941x::setDateTime(
  byte sec,        // 0-59
  byte min,        // 0-59
  byte hr,          // 1-23
  byte dyofWk,     // 1-7
  byte dyofMnth,    // 1-28/29/30/31
  byte mnth,         // 1-12
  byte yr)          // 0-99
{
  Wire.beginTransmission(MCP7941x_RTC_I2C_ADDR);
  WireSend(RTC_LOCATION);

  WireSend(decToBcd(sec) & 0x7f);              // set seconds and disable clock (01111111)
  WireSend(decToBcd(min) & 0x7f);              // set minutes (01111111)
  WireSend(decToBcd(hr) & 0x3f);                // set hours and to 24hr clock (00111111)
  WireSend(0x08 | (decToBcd(dyofWk) & 0x07));  // set the day and enable battery backup (00000111)|(00001000)
  WireSend(decToBcd(dyofMnth) & 0x3f);          // set the date in mnth (00111111)
  WireSend(decToBcd(mnth) & 0x1f);               // set the mnth (00011111)
  WireSend(decToBcd(yr));                       // set the yr (11111111)

  Wire.endTransmission();

  // Start Clock:
  Wire.beginTransmission(MCP7941x_RTC_I2C_ADDR);
  WireSend(RTC_LOCATION);
  WireSend(decToBcd(sec) | 0x80);     // set seconds and enable clock (10000000)
  Wire.endTransmission();
}


// Get the date/time:
void MCP7941x::getDateTime(
  byte *sec,
  byte *min,
  byte *hr,
  byte *dyofWk,
  byte *dyofMnth,
  byte *mnth,
  byte *yr)
{
  Wire.beginTransmission(MCP7941x_RTC_I2C_ADDR);
  WireSend(RTC_LOCATION);
  Wire.endTransmission();

  Wire.requestFrom(MCP7941x_RTC_I2C_ADDR, 7);

  // A few of these need masks because certain bits are control bits
  *sec     = bcdToDec(WireReceive() & 0x7f);  // 01111111
  *min     = bcdToDec(WireReceive() & 0x7f);  // 01111111
  *hr       = bcdToDec(WireReceive() & 0x3f);  // 00111111
  *dyofWk  = bcdToDec(WireReceive() & 0x07);  // 01111111
  *dyofMnth = bcdToDec(WireReceive() & 0x3f);  // 00111111
  *mnth      = bcdToDec(WireReceive() & 0x1f);  // 00011111
  *yr       = bcdToDec(WireReceive());         // 11111111
}


// Enable the clock without changing the date/time:
void MCP7941x::enableClock()
{
  // Get the current seconds value as the enable/disable bit is in the same
  // byte of memory as the seconds value:
  Wire.beginTransmission(MCP7941x_RTC_I2C_ADDR);
  WireSend(RTC_LOCATION);
  Wire.endTransmission();

  Wire.requestFrom(MCP7941x_RTC_I2C_ADDR, 1);

  int sec = bcdToDec(WireReceive() & 0x7f);  // 01111111

  // Start Clock:
  Wire.beginTransmission(MCP7941x_RTC_I2C_ADDR);
  WireSend(RTC_LOCATION);
  WireSend(decToBcd(sec) | 0x80);     // set seconds and enable clock (10000000)
  Wire.endTransmission();
}


// Disable the clock without changing the date/time:
void MCP7941x::disableClock()
{
  // Get the current seconds value as the enable/disable bit is in the same
  // byte of memory as the seconds value:
  Wire.beginTransmission(MCP7941x_RTC_I2C_ADDR);
  WireSend(RTC_LOCATION);
  Wire.endTransmission();

  Wire.requestFrom(MCP7941x_RTC_I2C_ADDR, 1);

  int sec = bcdToDec(WireReceive() & 0x7f);  // 01111111

  // Start Clock:
  Wire.beginTransmission(MCP7941x_RTC_I2C_ADDR);
  WireSend(RTC_LOCATION);
  WireSend(decToBcd(sec));     // set seconds and disable clock (01111111)
  Wire.endTransmission();
}



// Enable the battery:
void MCP7941x::enableBattery()
{
  // Get the current seconds value as the enable/disable bit is in the same
  // byte of memory as the seconds value:
  Wire.beginTransmission(MCP7941x_RTC_I2C_ADDR);
  WireSend(RTC_LOCATION + 0x03);
  Wire.endTransmission();

  Wire.requestFrom(MCP7941x_RTC_I2C_ADDR, 1);

  int day = bcdToDec(WireReceive() & 0x07);  // 00000111

  // Start Clock:
  Wire.beginTransmission(MCP7941x_RTC_I2C_ADDR);
  WireSend(RTC_LOCATION + 0x03);
  WireSend(decToBcd(day) | 0x08);     // set day and enable battery (00001000)
  Wire.endTransmission();
}


// Store byte of data in SRAM:
void MCP7941x::setSramByte ( byte location, byte data )
{
  if (location >= 0x20 && location <= 0x5f)
  {
    Wire.beginTransmission(MCP7941x_RTC_I2C_ADDR);
    WireSend(location);
    WireSend(data);
    Wire.endTransmission();
  }
}


// Read byte of data from SRAM:
byte MCP7941x::getSramByte ( byte location )
{
  if (location >= 0x20 && location <= 0x5f)
  {
    Wire.beginTransmission(MCP7941x_RTC_I2C_ADDR);
    WireSend(location);
    Wire.endTransmission();

    Wire.requestFrom(MCP7941x_RTC_I2C_ADDR, 1);

    return WireReceive();
  }
}

// New timer functions here

// Set the alarm 0 date/time, set to 24hr and reset Alarm Interrupt Flag bit:
// (assumes you're passing in valid numbers)
void MCP7941x::setAlarm0DateTime(
  byte sec,        // 0-59
  byte min,        // 0-59
  byte hr,          // 1-23
  byte dyofWk,     // 1-7
  byte dyofMnth,    // 1-28/29/30/31
  byte mnth)         // 1-12
{
// Read and save the weekday register
  Wire.beginTransmission(MCP7941x_RTC_I2C_ADDR);
  WireSend(WEEKDAY0_LOCATION);
  Wire.endTransmission();

  Wire.requestFrom(MCP7941x_RTC_I2C_ADDR, 1);

  byte wkday = WireReceive() & 0xF8; // (11111000)
//
  Wire.beginTransmission(MCP7941x_RTC_I2C_ADDR);
  WireSend(ALARM0_LOCATION);

  WireSend(decToBcd(sec) & 0x7f);              // set seconds (01111111)
  WireSend(decToBcd(min) & 0x7f);              // set minutes (01111111)
  WireSend(decToBcd(hr) & 0x3f);                // set hours and to 24hr clock (00111111)
  WireSend(wkday | (decToBcd(dyofWk) & 0x07));  // set the day and replace weekly register bits (00000111)
  WireSend(decToBcd(dyofMnth) & 0x3f);          // set the date in mnth (00111111)
  WireSend(decToBcd(mnth) & 0x1f);               // set the mnth (00011111)

  Wire.endTransmission();
}

// Set the alarm 0 date/time, set to 24hr and reset Alarm Interrupt Flag bit:
// (assumes you're passing in valid numbers)
void MCP7941x::setAlarm0UnixTime(int unixTime)
{
// Read and save the weekday register
  Wire.beginTransmission(MCP7941x_RTC_I2C_ADDR);
  WireSend(WEEKDAY0_LOCATION);
  Wire.endTransmission();

  Wire.requestFrom(MCP7941x_RTC_I2C_ADDR, 1);

  byte wkday = WireReceive() & 0xF8; // (11111000)

//
  Wire.beginTransmission(MCP7941x_RTC_I2C_ADDR);
  WireSend(ALARM0_LOCATION);

  WireSend(decToBcd(Time.second(unixTime)) & 0x7f);              // set seconds (01111111)
  WireSend(decToBcd(Time.minute(unixTime)) & 0x7f);              // set minutes (01111111)
  WireSend(decToBcd(Time.hour(unixTime)) & 0x3f);                // set hours and to 24hr clock (00111111)
  WireSend(wkday | (decToBcd(Time.weekday(unixTime)) & 0x07));  // set the day and replace weekly register bits (00000111)
  WireSend(decToBcd(Time.day(unixTime)) & 0x3f);          // set the date in mnth (00111111)
  WireSend(decToBcd(Time.month(unixTime)) & 0x1f);               // set the mnth (00011111)

  Wire.endTransmission();
}


// Get the date/time:
void MCP7941x::getAlarm0DateTime(
  byte *sec,
  byte *min,
  byte *hr,
  byte *dyofWk,
  byte *dyofMnth,
  byte *mnth)
{
  Wire.beginTransmission(MCP7941x_RTC_I2C_ADDR);
  WireSend(ALARM0_LOCATION);
  Wire.endTransmission();

  Wire.requestFrom(MCP7941x_RTC_I2C_ADDR, 7);

  // A few of these need masks because certain bits are control bits
  *sec     = bcdToDec(WireReceive() & 0x7f);  // 01111111
  *min     = bcdToDec(WireReceive() & 0x7f);  // 01111111
  *hr       = bcdToDec(WireReceive() & 0x3f);  // 00111111
  *dyofWk  = bcdToDec(WireReceive() & 0x07);  // 01111111
  *dyofMnth = bcdToDec(WireReceive() & 0x3f);  // 00111111
  *mnth      = bcdToDec(WireReceive() & 0x1f);  // 00011111

}


// Get the date/time:
int MCP7941x::getAlarm0UnixTime()
{
  byte sec;
  byte min;
  byte hr;
  byte dyofWk;
  byte dyofMnth;
  byte mnth;
  byte yr;

  Wire.beginTransmission(MCP7941x_RTC_I2C_ADDR);
  WireSend(ALARM0_LOCATION);
  Wire.endTransmission();

  Wire.requestFrom(MCP7941x_RTC_I2C_ADDR, 7);

  // A few of these need masks because certain bits are control bits
  sec     = bcdToDec(WireReceive() & 0x7f);  // 01111111
  min     = bcdToDec(WireReceive() & 0x7f);  // 01111111
  hr       = bcdToDec(WireReceive() & 0x3f);  // 00111111
  dyofWk  = bcdToDec(WireReceive() & 0x07);  // 01111111
  dyofMnth = bcdToDec(WireReceive() & 0x3f);  // 00111111
  mnth      = bcdToDec(WireReceive() & 0x1f);  // 00011111
  yr = 0;
  //declare variable
  struct tm tm;
  tm.tm_sec = (sec);
  tm.tm_min = (min);
  tm.tm_hour = (hr);
  tm.tm_mday = (dyofMnth);
  tm.tm_mon = (mnth)- 1;    // Assuming your month represents Jan with 1
  tm.tm_year = (yr);


//  delay(1000);
//  Particle.publish("Timer time:",String(mnth)+"/"+String(dyofMnth)+"/ "+String(hr-4)+":"+String(min)+":"+String(sec));
//  delay(1000);
  time_t moment = mktime(&tm);//create epoc time_t object

  return int(moment);
}

// Tested above

// Disable both the alarms without changing the control settings:
void MCP7941x::disableAlarms()
{
  // Get the current controls value as the enable/disable bits are in the same
  // byte of memory as the controls value:
  Wire.beginTransmission(MCP7941x_RTC_I2C_ADDR);
  WireSend(CONTROL_LOCATION);
  Wire.endTransmission();

  Wire.requestFrom(MCP7941x_RTC_I2C_ADDR, 1);

  int control = bcdToDec(WireReceive());

  // Start Clock:
  Wire.beginTransmission(MCP7941x_RTC_I2C_ADDR);
  WireSend(CONTROL_LOCATION);
  WireSend(decToBcd(control) & 0xCF);     // disable alarms (11001111)
  Wire.endTransmission();
}

// Disable alarm 0 without changing the control settings:
void MCP7941x::disableAlarm0()
{
  // Get the current controls value as the enable/disable bits are in the same
  // byte of memory as the controls value:
  Wire.beginTransmission(MCP7941x_RTC_I2C_ADDR);
  WireSend(CONTROL_LOCATION);
  Wire.endTransmission();

  Wire.requestFrom(MCP7941x_RTC_I2C_ADDR, 1);

  int control = bcdToDec(WireReceive());

  // Start Clock:
  Wire.beginTransmission(MCP7941x_RTC_I2C_ADDR);
  WireSend(CONTROL_LOCATION);
  WireSend(decToBcd(control) & 0xEF);     // disable alarm 0 (11101111)
  Wire.endTransmission();
}

// Disable alarm 1 without changing the control settings:
void MCP7941x::disableAlarm1()
{
  // Get the current controls value as the enable/disable bits are in the same
  // byte of memory as the controls value:
  Wire.beginTransmission(MCP7941x_RTC_I2C_ADDR);
  WireSend(CONTROL_LOCATION);
  Wire.endTransmission();

  Wire.requestFrom(MCP7941x_RTC_I2C_ADDR, 1);

  int control = bcdToDec(WireReceive());

  // Start Clock:
  Wire.beginTransmission(MCP7941x_RTC_I2C_ADDR);
  WireSend(CONTROL_LOCATION);
  WireSend(decToBcd(control) & 0xDF);     // disable alarm 1 (11011111)
  Wire.endTransmission();
}

// Enable both the alarms without changing the control settings:
void MCP7941x::enableAlarms()
{
  // Get the current controls value as the enable/disable bits are in the same
  // byte of memory as the controls value:
  Wire.beginTransmission(MCP7941x_RTC_I2C_ADDR);
  WireSend(CONTROL_LOCATION);
  Wire.endTransmission();

  Wire.requestFrom(MCP7941x_RTC_I2C_ADDR, 1);

  int control = bcdToDec(WireReceive());

  // Start Clock:
  Wire.beginTransmission(MCP7941x_RTC_I2C_ADDR);
  WireSend(CONTROL_LOCATION);
  WireSend(decToBcd(control) | 0x30);     // enable alarms (00110000)
  Wire.endTransmission();
}

// Enable alarm 0 without changing the control settings:
void MCP7941x::enableAlarm0()
{
  // Get the current controls value as the enable/disable bits are in the same
  // byte of memory as the controls value:
  Wire.beginTransmission(MCP7941x_RTC_I2C_ADDR);
  WireSend(CONTROL_LOCATION);
  Wire.endTransmission();

  Wire.requestFrom(MCP7941x_RTC_I2C_ADDR, 1);

  int control = bcdToDec(WireReceive());

  // Start Clock:
  Wire.beginTransmission(MCP7941x_RTC_I2C_ADDR);
  WireSend(CONTROL_LOCATION);
  WireSend(decToBcd(control) | 0x10);     // enable alarm 0 (00010000)
  Wire.endTransmission();
}

// Enable alarm 1 without changing the control settings:
void MCP7941x::enableAlarm1()
{
  // Get the current controls value as the enable/disable bits are in the same
  // byte of memory as the controls value:
  Wire.beginTransmission(MCP7941x_RTC_I2C_ADDR);
  WireSend(CONTROL_LOCATION);
  Wire.endTransmission();

  Wire.requestFrom(MCP7941x_RTC_I2C_ADDR, 1);

  int control = bcdToDec(WireReceive());

  // Start Clock:
  Wire.beginTransmission(MCP7941x_RTC_I2C_ADDR);
  WireSend(CONTROL_LOCATION);
  WireSend(decToBcd(control) | 0x20);     // enable alarm 1 (00100000)
  Wire.endTransmission();
}

// MFP high in general purpose mode
void MCP7941x::outHigh()
{
  // Get the current controls value as the enable/disable bits are in the same
  // byte of memory as the controls value:
  Wire.beginTransmission(MCP7941x_RTC_I2C_ADDR);
  WireSend(CONTROL_LOCATION);
  Wire.endTransmission();

  Wire.requestFrom(MCP7941x_RTC_I2C_ADDR, 1);

  int control = bcdToDec(WireReceive());

  // Start Clock:
  Wire.beginTransmission(MCP7941x_RTC_I2C_ADDR);
  WireSend(CONTROL_LOCATION);
  WireSend(decToBcd(control) | 0x80);     // MFP high in general purpose mode (10000000)
  Wire.endTransmission();
}

// MFP low in general purpose mode
void MCP7941x::outLow()
{
  // Get the current controls value as the enable/disable bits are in the same
  // byte of memory as the controls value:
  Wire.beginTransmission(MCP7941x_RTC_I2C_ADDR);
  WireSend(CONTROL_LOCATION);
  Wire.endTransmission();

  Wire.requestFrom(MCP7941x_RTC_I2C_ADDR, 1);

  int control = bcdToDec(WireReceive());

  // Start Clock:
  Wire.beginTransmission(MCP7941x_RTC_I2C_ADDR);
  WireSend(CONTROL_LOCATION);
  WireSend(decToBcd(control) | 0x7F);     // MFP low in general purpose mode (01111111)
  Wire.endTransmission();
}



/* Set ALMxWKDAY 0 without changing the register settings:
000 = Seconds match
001 = Minutes match
010 = Hours match (logic takes into account 12-/24-hour operation)
011 = Day of week match
100 = Date match
101 = Reserved; do not use
110 = Reserved; do not use
111 = Seconds, Minutes, Hour, Day of Week, Date and Month

Valid values for time-match include: Seconds, Minutes, Hours, Day, Date, All

Note that this also automatically clears ALM0IF (the Alarm Interrupt Flag bit)
*/
void MCP7941x::maskAlarm0(String tim_match)
{
  // Get the current controls value as the enable/disable bits are in the same
  // byte of memory as the controls value:
  byte mask;
  if (tim_match == String("Seconds"))
  {
  mask = 0x8F; // Seconds (10001111)
  }
  else if (tim_match == String("Minutes"))
  {
  mask = 0x9F; // Minutes (10011111)
  }
  else if (tim_match == String("Hours"))
  {
  mask = 0xAF; // Hours (10101111)
  }
  else if (tim_match == String("Day"))
  {
  mask = 0xBF; // Day of week (10111111)
  }
  else if (tim_match == String("Date"))
  {
  mask = 0xCF; // Date (11001111)
  }
  else if (tim_match == String("All"))
  {
  mask = 0xFF; // Seconds, Minutes, Hour, Day of Week, Date and Month (11111111)
  }
  else
  {
  return;
  }

  Wire.beginTransmission(MCP7941x_RTC_I2C_ADDR);
  WireSend(WEEKDAY0_LOCATION);
  Wire.endTransmission();

  Wire.requestFrom(MCP7941x_RTC_I2C_ADDR, 1);

  byte wkday = WireReceive();

  // Start Clock:
  Wire.beginTransmission(MCP7941x_RTC_I2C_ADDR);
  WireSend(WEEKDAY0_LOCATION);
  WireSend((wkday & 0x8F) | mask); // clear mask (weekday & 0x8F) then replace
  Wire.endTransmission();
}


/*
Clears ALM0IF (the Alarm Interrupt Flag bit)
without changing the register settings:
*/
void MCP7941x::clearIntAlarm0()
{

  Wire.beginTransmission(MCP7941x_RTC_I2C_ADDR);
  WireSend(WEEKDAY0_LOCATION);
  Wire.endTransmission();

  Wire.requestFrom(MCP7941x_RTC_I2C_ADDR, 1);

  byte wkday = WireReceive();

  // Start Clock:
  Wire.beginTransmission(MCP7941x_RTC_I2C_ADDR);
  WireSend(WEEKDAY0_LOCATION);
  WireSend(wkday & 0xF7); // (11110111)Simply writing clears the alarm interrupt
  Wire.endTransmission();
}


void MCP7941x::setAlarm0PolHigh()
{
  Wire.beginTransmission(MCP7941x_RTC_I2C_ADDR);
  WireSend(WEEKDAY0_LOCATION);
  Wire.endTransmission();

  Wire.requestFrom(MCP7941x_RTC_I2C_ADDR, 1);

  byte wkday = WireReceive();

  Wire.beginTransmission(MCP7941x_RTC_I2C_ADDR);
  WireSend(WEEKDAY0_LOCATION);
  WireSend(wkday | 0x80); // set ALMPOL to 1 (10000000)
  Wire.endTransmission();

}

void MCP7941x::setAlarm0PolLow()
{
  Wire.beginTransmission(MCP7941x_RTC_I2C_ADDR);
  WireSend(WEEKDAY0_LOCATION);
  Wire.endTransmission();

  Wire.requestFrom(MCP7941x_RTC_I2C_ADDR, 1);

  byte wkday = WireReceive();

  Wire.beginTransmission(MCP7941x_RTC_I2C_ADDR);
  WireSend(WEEKDAY0_LOCATION);
  WireSend(wkday & 0x7F); // set ALMPOL to 0 (01111111)
  Wire.endTransmission();

}


// Set the date/time, set to 24hr and enable the clock:
// (assumes you're passing in valid numbers)
void MCP7941x::setUnixTime(uint32_t unixTime)
{
  Wire.beginTransmission(MCP7941x_RTC_I2C_ADDR);
  WireSend(RTC_LOCATION);

  WireSend(decToBcd(Time.second(unixTime)) & 0x7f);              // set seconds and disable clock (01111111)
  WireSend(decToBcd(Time.minute(unixTime)) & 0x7f);              // set minutes (01111111)
  WireSend(decToBcd(Time.hour(unixTime)) & 0x3f);                // set hours and to 24hr clock (00111111)
  WireSend(0x08 | (decToBcd(Time.weekday(unixTime)) & 0x07));  // set the day and enable battery backup (00000111)|(00001000)
  WireSend(decToBcd(Time.day(unixTime)) & 0x3f);          // set the date in mnth (00111111)
  WireSend(decToBcd(Time.month(unixTime)) & 0x1f);               // set the mnth (00011111)
  WireSend(decToBcd(Time.year(unixTime)));                       // set the yr (11111111)

  Wire.endTransmission();

  // Start Clock:
  Wire.beginTransmission(MCP7941x_RTC_I2C_ADDR);
  WireSend(RTC_LOCATION);
  WireSend(decToBcd(Time.second(unixTime)) | 0x80);     // set seconds and enable clock (10000000)
  Wire.endTransmission();
}


// Get the date/time:
void MCP7941x::publishAlarm0Debug()
{
  Wire.beginTransmission(MCP7941x_RTC_I2C_ADDR);
  WireSend(ALARM0_LOCATION);
  Wire.endTransmission();

  Wire.requestFrom(MCP7941x_RTC_I2C_ADDR, 6);

  // A few of these need masks because certain bits are control bits


  String sec     = String(bcdToDec(WireReceive() & 0x7f));  // 01111111
  String min     = String(bcdToDec(WireReceive() & 0x7f));  // 01111111
  byte hrRaw = WireReceive();
  String hr       = String(bcdToDec(hrRaw & 0x3f));  // 00111111
  String hrReg = String(hrRaw & 0x40);  // 01000000
  byte dyofWkRaw = WireReceive();
  String dyofWk  = String(bcdToDec(dyofWkRaw & 0x07));  // 00000111
  String dyofWkReg = String(dyofWkRaw & 0xF8);  // 11111000
  String dyofMnth = String(bcdToDec(WireReceive() & 0x3f));  // 00111111
  String mnth      = String(bcdToDec(WireReceive() & 0x1f));  // 00011111

  Wire.beginTransmission(MCP7941x_RTC_I2C_ADDR);
  WireSend(CONTROL_LOCATION);
  Wire.endTransmission();

  Wire.requestFrom(MCP7941x_RTC_I2C_ADDR, 1);

  String contrl = String(WireReceive() & 0x30); // (00110000)

  String debugString = String("MM:dd:dw:hr:mm:ss"+mnth+":"+dyofMnth+":"+dyofWk+":"+hr+":"+min+":"+sec+":"+"Control:"+contrl+"dyofWkReg"+dyofWkReg+"hrReg"+hrReg);

  Particle.publish("Alarm Debug", debugString);

}


uint32_t MCP7941x::rtcNow(){
  //gets unix as defined by the external RTC

  byte sec;
  byte min;
  byte hr;
  byte dyofWk;
  byte dyofMnth;
  byte mnth;
  byte yr;

  //get the Date and time as bytes
  getDateTime(&sec,
		  &min,
		  &hr,
		  &dyofWk,
		  &dyofMnth,
		  &mnth,
		  &yr);



  //declare variable
  struct tm tm;
  tm.tm_sec = (sec);
  tm.tm_min = (min);
  tm.tm_hour = (hr);
  tm.tm_mday = (dyofMnth);
  tm.tm_mon = (mnth)- 1;    // Assuming your month represents Jan with 1
  tm.tm_year = yr + 52; // for some reason... the clock is set with this year ofset....

  time_t moment = mktime(&tm);//create epoc time_t object

  return uint32_t(moment);
}
