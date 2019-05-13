

#include "application.h"
#include "IoTNodePower.h"
#include <Adafruit_MCP23017.h>

Adafruit_MCP23017 IOexp;



// Constructor
IoTNodePower::IoTNodePower()
    {

    }

void IoTNodePower::begin()
{

      IOexp.begin();
      //Set pin direction 1 = out, 0 = in
      //PORT_A,0b10111111 | PORT_B,0b00001111
      IOexp.pinMode(0,OUTPUT);
      IOexp.pinMode(1,OUTPUT);
      IOexp.pinMode(2,OUTPUT);
      IOexp.pinMode(3,OUTPUT);
      IOexp.pinMode(4,OUTPUT);
      IOexp.pinMode(5,OUTPUT);
      IOexp.pinMode(6,INPUT);
      IOexp.pinMode(7,OUTPUT);
      IOexp.pinMode(8,INPUT);
      IOexp.pinMode(9,INPUT);
      IOexp.pinMode(10,INPUT);
      IOexp.pinMode(11,INPUT);
      IOexp.pinMode(12,INPUT);
      IOexp.pinMode(13,INPUT);
      IOexp.pinMode(14,INPUT);
      IOexp.pinMode(15,INPUT);


      IOexp.pullUp(0,HIGH);
      IOexp.pullUp(1,HIGH);
      IOexp.pullUp(2,HIGH);
      IOexp.pullUp(3,HIGH);
      IOexp.pullUp(4,HIGH);
      IOexp.pullUp(5,HIGH);
      IOexp.pullUp(6,HIGH);
      IOexp.pullUp(7,HIGH);
      IOexp.pullUp(8,HIGH);
      IOexp.pullUp(9,HIGH);
      IOexp.pullUp(10,HIGH);
      IOexp.pullUp(11,HIGH);
      IOexp.pullUp(12,HIGH);
      IOexp.pullUp(13,HIGH);
      IOexp.pullUp(14,HIGH);
      IOexp.pullUp(15,HIGH);

      //IOexp.writeGPIOAB(0);
}

void IoTNodePower::setPowerON(powerName pwrName, bool state)
    {
        IOexp.digitalWrite(pwrName, state);
        //i.e. IOexp.digitalWrite(3, state);
    }

void IoTNodePower::tickleWatchdog()
{
  IOexp.digitalWrite(5,true);
  //delayMicroseconds(100);
  delay(50);
  IOexp.digitalWrite(5,false);
}

bool IoTNodePower::isLiPoPowered()
{
// uint8_t digitalRead(uint8_t p);
  if(IOexp.digitalRead(10)==0)
  {
    return true;
  }
  else
  {
    return false;
  }
}

bool IoTNodePower::is3AAPowered()
{
// uint8_t digitalRead(uint8_t p);
  if(IOexp.digitalRead(10)==1)
  {
    return true;
  }
  else
  {
    return false;
  }
}

bool IoTNodePower::isLiPoCharged()
{
// uint8_t digitalRead(uint8_t p);
  if(IOexp.digitalRead(8)==0)
  {
    return true;
  }
  else
  {
    return false;
  }
}

bool IoTNodePower::isLiPoCharging()
{
// uint8_t digitalRead(uint8_t p);
  if(IOexp.digitalRead(9)==0)
  {
    return true;
  }
  else
  {
    return false;
  }
}

float IoTNodePower::voltage()
{
    unsigned int rawVoltage = 0;
    float voltage = 0.0;
    Wire.requestFrom(0x4D, 2);
    if (Wire.available() == 2)
    {
    	rawVoltage = (Wire.read() << 8) | (Wire.read());
      voltage = (float)(rawVoltage)/4096.0*13.64; // 3.3*(4.7+1.5)/1.5
    }
    return voltage;
}
