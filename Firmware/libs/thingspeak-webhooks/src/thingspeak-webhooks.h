/*
    Copyright (c) 2018 Sentient Things, Inc.  All right reserved.
*/



// ensure this library description is only included once
#ifndef ThingSpeakWebhooks_h
#define ThingSpeakWebhooks_h

// include Particle library
#include "application.h"

// library interface description
class ThingSpeakWebhooks
{
  // user-accessible "public" interface
  public:
    ThingSpeakWebhooks();
    // Note the new 622 character size limitation of Particle.publish() messages
    void TSBulkWriteCSV(String channel, String api_key, String time_format, String csvData);
    boolean TSCreateChan(char const* keys[], char const* values[], int& returnIndex);
    boolean updateTSChan(char const* channel, char const* values[], char const* labels[], int& arrayIndex);
    void TSWriteOneSetting(int channelNum, String value, String label);
  // library-accessible "private" interface
  private:
    int value;
    String urlencode(String str);
};

#endif
