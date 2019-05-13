/*
  MCP7941x.h - Arduino Library for using the MCP7941x IC.
  Ian Chilton <ian@ichilton.co.uk>
  November 2011

  Modified for use for the particle.io devices
  by Robert Mawrey
  June 2016

*/

#ifndef MCP7941x_h
#define MCP7941x_h

#define MCP7941x_EEPROM_I2C_ADDR 0x57
#define MAC_LOCATION 0xF2   // Starts at 0xF0 but we are only interested in 6 bytes.

#define MCP7941x_RTC_I2C_ADDR 0x6F
#define RTC_LOCATION 0x00

// New timer defines

#define WEEKDAY0_LOCATION 0x0D
#define WEEKDAY1_LOCATION 0x14
#define ALARM0_LOCATION 0x0A
#define ALARM1_LOCATION 0x11
#define CONTROL_LOCATION 0x07

//Removed to work with the particle.io devices
/*
  #if (ARDUINO >= 100)
    #include <Arduino.h>
  #else
    #include <WProgram.h>
  #endif
*/

// Added for the particle.io devices
#include "application.h"

  class MCP7941x
  {
    public:

      MCP7941x();

      byte decToBcd ( byte val );
      byte bcdToDec ( byte val );

      void getMacAddress ( byte *mac_address );
      void unlockUniqueID();
      void writeMacAddress ( byte *mac_address );

      void setDateTime ( byte sec, byte min, byte hr, byte dayofWk, byte dayofMnth, byte mnth, byte yr );
      void getDateTime ( byte *sec, byte *minute, byte *hr, byte *dayofWk, byte *dayofMnth, byte *mnth, byte *yr );
      void enableClock ();
      void disableClock ();
      void enableBattery ();

      void setSramByte ( byte location, byte data );
      byte getSramByte ( byte location );

      // New timer functions here

      void setUnixTime (uint32_t unixTime);

      void setAlarm0DateTime( byte sec, byte minute, byte hr, byte dayofWk, byte dayofMnth, byte mnth);
      void setAlarm0UnixTime(int unixTime);

      void getAlarm0DateTime( byte *sec, byte *minute, byte *hr, byte *dayofWk, byte *dayofMnth, byte *mnth);

      int getAlarm0UnixTime();

      void disableAlarms();
      void disableAlarm0();
      void disableAlarm1();
      void enableAlarms();
      void enableAlarm0();
      void enableAlarm1();

      void maskAlarm0(String time_match);
      void clearIntAlarm0();

      void setAlarm0PolHigh();
      void setAlarm0PolLow();

      void outHigh();

      void outLow();

      void publishAlarm0Debug();
      // Added by Mawrey
      uint32_t rtcNow();

    private:

  };

#endif
