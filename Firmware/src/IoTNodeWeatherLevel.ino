/*
 * Project IoTNodeWeatherLevel
 * Description: Weather station and tide level monitor. Measures Weather
   and tide level data and sends the results to Thingspeak.com
 * Author: Robert Mawrey, Sentient Things, Inc.
 * Date: January 2019
 */
#include "Particle.h"
#include <IoTNodePower.h>
#include <WeatherLevel.h>
#include <thingspeak-webhooks.h>
#include <IoTNodeWeatherLevelGlobals.h>
#include <ArduinoJson.h>
#include <MCP7941x.h>
#include <FramI2C.h>



// // SdFat and SdCardLogHandler does not currently work with Particle Mesh devices
// ///------
// //#include <SdFat.h> // There is currently a bug in this version of SdFat for gen 3 Particle
// #include "SdFat.h"
// #include "SdCardLogHandlerRK.h" //Reverted to local version as public uses old SdFat.h

// // Consider adding SD and logging to IoTNode library

// const int SD_CHIP_SELECT = N_D0;
// SdFat sd;
// SdCardLogHandler logHandler(sd, SD_CHIP_SELECT, SPI_FULL_SPEED);

// STARTUP(logHandler.withNoSerialLogging().withMaxFilesToKeep(3000));
// ///------

#define SENSOR_SEND_TIME_MS 60000
#define SENSOR_POLL_TIME_MS 2000

#define IOTDEBUG

const int firmwareVersion = 0;

//SYSTEM_THREAD(ENABLED);

// Using SEMI_AUTOMATIC mode to get the lowest possible data usage by
// registering functions and variables BEFORE connecting to the cloud.
//SYSTEM_MODE(SEMI_AUTOMATIC);

// Use structs defined in IoTNodeWeatherLevelGlobals.h
sensorReadings_t sensorReadings;
config_t config;
status_t status;

// ThingSpeak webhook keys. Do not edit.
char const* webhookKey[] = {
    "d", // "description=",
    "e", // "elevation=",
    "1", // "field1=",
    "2", // "field2=",
    "3", // "field3=",
    "4", // "field4=",
    "5", // "field5=",
    "6", // "field6=",
    "7", // "field7=",
    "8", // "field8=",
    "a", // "latitude=",
    "o", // "longitude=",
    "n", // "name=",
    "f", // "public_flag=",
    "t", // "tags=",
    "u", // "url=",
    "m", // "metadata=",
    // "api_key=",
    "end" //Do not remove or change
};

// ThingSpeak create Channel labels. Do not edit.
char const* chanLabels[] = {
    "description",
    "elevation",
    "field1",
    "field2",
    "field3",
    "field4",
    "field5",
    "field6",
    "field7",
    "field8",
    "latitude",
    "longitude",
    "name",
    "public_flag",
    "tags",
    "url",
    "metadata",
        // "api_key=",
    "end" //Do not remove or change
};

//Sensor Names
char const* sensorNames[] = {
  "Weather and level station using the Sentient Things IoT Node",   // "description=",
  "",                       // "elevation=",
  "Wind Direction deg.",    // "field1=",
  "Wind Speed mph",         // "field2=",
  "Humidity %",             // "field3=",
  "Air Temperature F",      // "field4=",
  "Rainfall in.",           // "field5=",
  "Air Pressure in.",       // "field6=",
  "Depth ft.",              // "field7" = reference - measured range
  "Water or Ground Temp. F",    // "field8=",
  "",                       // "latitude=",
  "",                       // "longitude=",
  "Weather and level station", // "name=",
  "false",                  // "public_flag=",
  "",                      // "tags=",
  "http://sentientthings.com",                       // "url=",
  "Note - Snow or water depth = Range Reference - measured range",   // "metadata=",
  "end"
};

// This is the index for the updateTSChan
int returnIndex;

byte messageSize = 1;

Timer pollSensorTimer(SENSOR_POLL_TIME_MS, capturePollSensors);

Timer sensorSendTimer(SENSOR_SEND_TIME_MS, getResetAndSendSensors);

WeatherLevel sensors; //Interrupts for anemometer and rain bucket
// are set up here too

// IoTNode iotnode;

IoTNodePower power;

ThingSpeakWebhooks thingspeak;

// Create FRAM instances
#define PART_NUMBER MB85RC256B
FramI2C myFram(PART_NUMBER);
framResult myResult = framUnknownError;
FramI2CArray framStatus(myFram, 1, sizeof(status), myResult);
FramI2CArray framConfig(myFram, 1, sizeof(config), myResult);

// Ring_FramArray dataRing(myFram, config.maxReadings, sizeof(sensorReadings), myResult);
Ring_FramArray dataRing(myFram, 300, sizeof(sensorReadings), myResult);

// Create new instance of RTC:
MCP7941x rtc;


bool readyToGetResetAndSendSensors = false;
bool readyToCapturePollSensors = false;
bool tickleWD = false;

unsigned long timeToNextSendMS;

// Change this value to force hard reset and clearing of FRAM when Flashing
const int firstRunTest = 1122121;

String deviceStatus;
String i2cDevices;
bool resetDevice = false;

// setup() runs once, when the device is first turned on.
void setup() {
  // register cloudy things
  Particle.function("rangemm", getRangemm);
  Particle.function("rangetype", getRangeType);
  Particle.variable("version",firmwareVersion);
  Particle.variable("devicestatus",deviceStatus);


  // Subscribe to the TSBulkWriteCSV response event
  Particle.subscribe(System.deviceID() + "/hook-response/TSBulkWriteCSV", TSBulkWriteCSVHandler, MY_DEVICES);
// Put a test in here for first run
  // Subscribe to the TSCreateChannel response event
  Particle.subscribe(System.deviceID() + "/hook-response/TSCreateChannel", TSCreateChannelHandler, MY_DEVICES);

  // etc...
  // then connect
  //Particle.connect();
  Serial.begin(115200);
  Serial1.begin(9600);
  #ifdef IOTDEBUG
  delay(5000);
  #endif
  // iotnode.begin();
  power.begin();
  power.setPowerON(EXT3V3,true);
  power.setPowerON(EXT5V,true); 
  // iotnode.setPowerON(EXT3V3,true);
  // iotnode.setPowerON(EXT5V,true);
  // iotnode.getConfig();
  framConfig.readElement(0, (uint8_t*)&config, myResult);

  // Code for initialization of the IoT Node
  // i.e. the first time the software runs
  // 1. A new ThingSpeak channel is created
  // 2. The channel id and keys are Saved
  // 3. a firstRunTest variable is saved in persistent memory as a flag to indicate
  // that the IoT node has been set up already.

  if (config.testCheck != firstRunTest)
  {
//boolean TSCreateChan(char const* keys[], char const* values[], int& returnIndex);
    // iotnode.framFormat();
    myFram.format();
    thingspeak.TSCreateChan(webhookKey,sensorNames, returnIndex);
    unsigned long webhookTime = millis();
    // iotnode.getConfig();
    framConfig.readElement(0, (uint8_t*)&config, myResult);
    config.rangeReferencemm = 10000;
    while (config.testCheck != firstRunTest && millis()-webhookTime<60000)
    {
      //Particle.publish("Trying to create ThingSpeak channel");
      delay(5000);
      // iotnode.getConfig();
      framConfig.readElement(0, (uint8_t*)&config, myResult);
      config.rangeReferencemm = 10000;
      // iotnode.saveConfig();
      framConfig.writeElement(0, (uint8_t*)&config, myResult);
    }
    System.reset();
  }

  // end of first run code.

  // iotnode.getStatus();
  framStatus.readElement(0, (uint8_t*)&status, myResult);
  // The range reference is set at a default of 10000mm when the TS channel is created
  // Particle function getRangemm is used to set it for the actual installation
  sensorReadings.rangeReferencemm = config.rangeReferencemm;

  // iotnode.saveConfig();
  framConfig.writeElement(0, (uint8_t*)&config, myResult);

  if (syncRTC())
  {
    Serial.println("RTC sync'ed with cloud");
  }
  else
  {
    Serial.println("RTC not sync'ed with cloud");
  }
/////
// Wake up the AM2315 sensor
// Wire.beginTransmission(0x5c);
// delay(2);
// Wire.endTransmission();
//
  
//#ifdef IOTDEBUG
  byte error, address;
  int nDevices;
  int i2cTime;

  Serial.println("Scanning...");
  i2cTime=millis();
  nDevices = 0;

  for(address = 1; address < 127; address++ )
  {
    // The i2c_scanner uses the return value of
    // the Write.endTransmisstion to see if
    // a device did acknowledge to the address.
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0)
    {
      Serial.print("I2C device found at address 0x");
      if (address<16)
      {
      Serial.print("0");
      i2cDevices.concat("0");
      }
      String addr = String(address,HEX);
      // Serial.print(address,HEX);
      Serial.print(addr);
      i2cDevices.concat(addr);
      i2cDevices.concat(" ");
      Serial.println("  !");

      nDevices++;
    }
    else if (error==4)
    {
      Serial.print("Unknown error at address 0x");
      if (address<16)
        Serial.print("0");
      Serial.println(address,HEX);
    }
  }
  if (nDevices == 0)
    Serial.println("No I2C devices found\n");
  else
  Serial.println("done\n");
  Serial.println(millis()-i2cTime);

  Serial.println(sizeof(sensorReadings));

  delay(5000);           // wait 5 seconds for next scan
//#endif

  sensors.begin();
  pollSensorTimer.start();
  sensorSendTimer.start();

/////
}


void loop() {
  if (readyToGetResetAndSendSensors)
  {
    sensors.getAndResetAllSensors();
    String lastCsvData = "";
    messageSize = 1;
    if (!dataRing.isEmpty())
    {
      sensorReadings_t temporaryReadings;
      // iotnode.peekLastSensorReadingsFromFRAM((uint8_t*)&temporaryReadings);
      dataRing.peekLastElement((uint8_t*)&temporaryReadings);
      lastCsvData = "|" + sensors.sensorReadingsToCsvUS(temporaryReadings);
      messageSize = 2;
    }
    // iotnode.pushSensorReadingsToFRAM(); //Remember to update indices
    dataRing.pushElement((uint8_t*)&sensorReadings);
    String currentCsvData = sensors.sensorReadingsToCsvUS();

    // Consider putting the SD logging in the IoTNode library
    // Log.info(currentCsvData);

    String csvData = currentCsvData + lastCsvData;
    #ifdef IOTDEBUG
    Serial.println();
    Serial.println(csvData);
    Serial.println(String(config.channelId));
    Serial.println(String(config.writeKey));
    #endif
    String time_format = "absolute";
    if (waitFor(Particle.connected,config.particleTimeout)){
      // Update TSBulkWriteCSV later to use chars
      thingspeak.TSBulkWriteCSV(String(config.channelId), String(config.writeKey), time_format, csvData);
    }
    else
    {
      Serial.println("Timeout");
    }

    // iotnode.saveStatus();
    dataRing.getIndices(&status.ringStart,&status.ringEnd);
    framStatus.writeElement(0, (uint8_t*)&status, myResult);
    readyToGetResetAndSendSensors = false;

    if (tickleWD)
    {
      // iotnode.tickleWatchdog();
      power.tickleWatchdog();
      tickleWD = false;
    }

    readyToGetResetAndSendSensors = false;
    #ifdef IOTDEBUG
    Serial.println("readyToGetResetAndSendSensors");
    #endif
    // Update status information
    deviceStatus = 
    String(config.channelId)+"|"+
    String(config.testCheck)+"|"+
    String(config.writeKey)+"|"+
    String(config.readKey)+"|"+
    String(config.maxbotixType)+"|"+
    String(config.unitType)+"|"+
    String(config.firmwareVersion)+"|"+
    String(config.particleTimeout)+"|"+
    String(config.rangeReferencemm)+"|"+
    String(config.latitude)+"|"+
    String(config.longitude)+"|"+
    i2cDevices;

  }

  if (readyToCapturePollSensors)
  {
    sensors.captureTempHumidityPressure();
    sensors.captureWindVane();
    readyToCapturePollSensors = false;
    #ifdef IOTDEBUG
    Serial.println("capture");
    #endif
  }
// If flag set then reset here
  if (resetDevice)
  {
    System.reset();
  }

}

void serialEvent1()
{
  // Process serialEvent1
  // Can also be done outside the thread
  sensors.parseMaxbotixToBuffer();
}

void capturePollSensors()
{
  // Set the flag to poll the sensors
  readyToCapturePollSensors = true;
}

void getResetAndSendSensors()
{
  // Set the flag to read and send data.
  // Has to be done out of this Timer thread
  timeToNextSendMS = millis();
  readyToGetResetAndSendSensors = true;
}

int getRangemm(String rangemm)
{
#ifdef IOTDEBUG
  Serial.println(rangemm);
#endif
  config.rangeReferencemm = rangemm.toInt();
  // iotnode.saveConfig();
  framConfig.writeElement(0, (uint8_t*)&config, myResult);
  delay(20);
  resetDevice = true;
  // System.reset();
  return 1;
}

int getRangeType(String rangeType)
{
#ifdef IOTDEBUG
  Serial.println(rangeType);
#endif
  char type = rangeType.charAt(0);
  if (type == 'i' || type == 'c' || type == 'm')
  {
    config.maxbotixType = rangeType.charAt(0);
    // iotnode.saveConfig();
    framConfig.writeElement(0, (uint8_t*)&config, myResult);
    delay(20);
    resetDevice = true;
    // System.reset();
    return 1;
  }
  else
  {
    return -1;
  }
}

void TSBulkWriteCSVHandler(const char *event, const char *data) {
  timeToNextSendMS = SENSOR_SEND_TIME_MS - (millis() - timeToNextSendMS);
  String resp = "true";
  if (resp.equals(String(data)))
  {
    sensorReadings_t temporaryReadings;
    if (messageSize == 2)
    {
      // iotnode.popLastSensorReadingsFromFRAM((uint8_t*)&temporaryReadings);
      // iotnode.popLastSensorReadingsFromFRAM((uint8_t*)&temporaryReadings);
      dataRing.popLastElement((uint8_t*)&temporaryReadings);
      dataRing.popLastElement((uint8_t*)&temporaryReadings);
    }
    else
    {
      // iotnode.popLastSensorReadingsFromFRAM((uint8_t*)&temporaryReadings);
      dataRing.popLastElement((uint8_t*)&temporaryReadings);
    }

    // iotnode.saveStatus();
    dataRing.getIndices(&status.ringStart,&status.ringEnd);
    framStatus.writeElement(0, (uint8_t*)&status, myResult);
    #ifdef IOTDEBUG
    Serial.println(data);
    #endif
  }
  tickleWD = true;
  //long sleepTime = (long)((timeToNextSendMS-1000)/1000-1);
  //System.sleep(sleepTime);
}

void TSCreateChannelHandler(const char *event, const char *data) {
  // Handle the TSCreateChannel response
  StaticJsonBuffer<256> jb;
  #ifdef IOTDEBUG
  Serial.println(data);
  #endif
  //JsonObject& obj = jb.parseObject((char*)data);
  JsonObject& obj = jb.parseObject(data);
  if (obj.success()) {
      int channelId = obj["i"];
      const char* write_key = obj["w"];
      const char* read_key = obj["r"];
      // Copy to config
      config.channelId = channelId;
      strcpy(config.writeKey,write_key);
      strcpy(config.readKey,read_key);
      config.testCheck = firstRunTest;
           /// Defaults
      config.maxbotixType = 'm';
      // config.maxReadings = 300;
      config.particleTimeout = 20000;
      // Add code for this to be updated as a variable
      config.rangeReferencemm = 0;
      // Save to FRAM
      // iotnode.saveConfig();
      framConfig.writeElement(0, (uint8_t*)&config, myResult);
      #ifdef IOTDEBUG
      Serial.println(channelId);
      Serial.println(write_key);
      Serial.println(read_key);
      #endif
      int len = sizeof(channelId)*8+1;
      char buf[len];
      char const* chan = itoa(channelId,buf,10);
      thingspeak.updateTSChan(chan,sensorNames,chanLabels,returnIndex);
      String chanTags = "weather, level, wind, temperature, humidity, pressure, Sentient Things," + System.deviceID();
      String lab = "tags";
      delay(1001);
      thingspeak.TSWriteOneSetting(channelId, chanTags, lab);
 
      ///
  } else {
      Serial.println("Parse failed");
      Particle.publish("Parse failed",data);
  }

}

bool syncRTC()
{
    uint32_t syncNow;
    bool sync = false;
    //Particle.syncTime();

    unsigned long syncTimer = millis();

    do
    {
      Particle.process();
      delay(100);
    } while (Time.now() < 1465823822 && millis()-syncTimer<500);

    if (Time.now() > 1465823822)
    {
        syncNow = Time.now();//put time into memory
        rtc.setUnixTime(syncNow);  //Particle.publish("TIME","Synced");
        sync = true;
    }

    if (!sync)
    {
        #ifdef DEBUG
        Particle.publish("Time NOT synced",String(Time.format(syncNow, TIME_FORMAT_ISO8601_FULL)+"  "+Time.format(rtc.rtcNow(), TIME_FORMAT_ISO8601_FULL)));
        #endif
    }
    return sync;
}
