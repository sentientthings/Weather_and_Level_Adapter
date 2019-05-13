// This #include statement was automatically added by the Particle IDE.
#include "MCP7941x.h"

MCP7941x rtc;

void setup() {
    Particle.syncTime();

    // Wait for time to be synchronized with Particle Device Cloud (requires active connection)
    waitFor(Time.isValid, 60000);

    // Set RTC
    rtc.setUnixTime(Time.now());

}

void loop() {
    // Read and publish time from the RTC
    Particle.publish("The real time clock UTC time is", Time.format(rtc.rtcNow(), TIME_FORMAT_DEFAULT));
    delay(10000);

}
