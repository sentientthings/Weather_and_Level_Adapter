#ifndef _IOTNODEWEATHERLEVEL
#define _IOTNODEWEATHERLEVEL

// Globals defined here

// #define PLATFORM_SPARK_CORE                 0
// #define PLATFORM_SPARK_CORE_HD              2
// #define PLATFORM_GCC                        3
// #define PLATFORM_PHOTON_DEV                 4
// #define PLATFORM_TEACUP_PIGTAIL_DEV         5
// #define PLATFORM_PHOTON_PRODUCTION          6
// #define PLATFORM_TEACUP_PIGTAIL_PRODUCTION  7
// #define PLATFORM_P1                         8
// #define PLATFORM_ETHERNET_PROTO             9
// #define PLATFORM_ELECTRON_PRODUCTION        10
// #define PLATFORM_ARGON						           12
// #define PLATFORM_BORON						           13
// #define PLATFORM_XENON						           14
// #define PLATFORM_NEWHAL                     60000
// 
//Photon
#if PLATFORM_ID==6 // || PLATFORM_ID==2
#define N_SDA0 SDA
#define N_SCL0 SCL
#define N_D0 D2
#define GIOA D2
#define N_D1 D3
#define GIOB D3
#define N_D2 D4
#define GIOC D4
#define N_D3 D5
#define GIOD D5
#define N_D4 D6
#define GIOE D6
#define N_D5 D7
#define GIOF D7
#define N_D6 DAC
#define GIOG DAC
#define N_A2 A0
#define N_A3 A1
#define N_SCK SCK
#define N_MOSI MOSI
#define N_MISO MISO
#define N_RX0 RX
#define N_TX0 TX
#endif

//Electron
#if PLATFORM_ID==10
#define N_SDA0 SDA
#define N_SCL0 SCL
#define N_D0 D2
#define GIOA D2
#define N_D1 D3
#define GIOB D3
#define N_D2 D4
#define GIOC D4
#define N_D3 D5
#define GIOD D5
#define N_D4 D6
#define GIOE D6
#define N_D5 D7
#define GIOF D7
#define N_D6 DAC
#define GIOG DAC
#define N_A0 B4
#define N_A1 B5
#define N_A2 A0
#define N_A3 A1
#define N_SCK SCK
#define N_MOSI MOSI
#define N_MISO MISO
#define N_RX0 RX
#define N_TX0 TX
#define N_RX1 UART4_RX
#define N_TX1 UART4_TX
#define CANRX CAN1_RX
#define CANTX CAN1_TX
#endif

//Mesh
#if PLATFORM_ID==12 || PLATFORM_ID==13 || PLATFORM_ID==14
#define N_SDA0 SDA
#define N_SCL0 SCL
#define N_D0 D2
#define GIOA D2
#define N_D1 D3
#define GIOB D3
#define N_D2 D4
#define GIOC D4
#define N_D3 D5
#define GIOD D5
#define N_D4 D6
#define GIOE D6
#define N_D5 D7
#define GIOF D7
#define N_D6 D8
#define GIOG D8
#define N_A0 A0
#define N_A1 A1
#define N_A2 A2
#define N_A3 A3
#define N_A4 A4
#define N_A5 A5
#define N_SCK SCK
#define N_MOSI MOSI
#define N_MISO MISO
#define N_RX0 RX
#define N_TX0 TX
#endif

typedef struct // units chosen for data size and readability
{
    //Weather
    uint32_t unixTime; //system_tick_t (uint32_t)
    uint16_t windDegrees; // 1 degree resolution is plenty
    uint16_t wind_metersph; //meters per hour
    uint8_t humid; //percent
    uint16_t airTempKx10; // Temperature in deciKelvin
    uint16_t rainmm; // millimeters
    float barometerhPa; // Could fit into smaller type if needed
    uint16_t gust_metersph; //meters per hour
    // Water
    uint16_t rangemm; //Range in millimeters
    uint16_t rangeReferencemm; // Range datum i.e. could be mllw range for tide
    uint16_t waterTempKx10; // Temperature in deciKelvin

}sensorReadings_t;
extern sensorReadings_t sensorReadings;

//struct to save created TS channel Id and keys and to check "first run"
typedef struct
{
  int channelId;
  int testCheck;
  char writeKey[17];
  char readKey[17];
  char maxbotixType; // i = inches, m = mm, c = cm
  char unitType; // U = USA, I = international
  int firmwareVersion;
  // int maxReadings;//=dataRing size
  int particleTimeout;
  uint16_t rangeReferencemm;
  float latitude;
  float longitude;
}config_t;
extern config_t config;

// These variables change as the device logs
typedef struct
{
    // Indices for the FRAM ring buffer.  Saved in FRAM for persistence
    // during power cycling and battery loss.
    unsigned long ringStart;
    unsigned long ringEnd;
}status_t;
extern status_t status;

//End of Globals
#endif
