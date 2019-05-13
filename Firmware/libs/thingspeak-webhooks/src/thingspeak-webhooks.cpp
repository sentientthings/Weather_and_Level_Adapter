/*
    Copyright (c) 2018 Sentient Things, Inc.  All right reserved.
*/

//#DEFINE DEBUG

// include this library's description file
#include "thingspeak-webhooks.h"

enum State {
    START,
    ADD_NEXT,
    CREATE_CHANNEL,
    //UPDATE_CHANNEL
};

State state = START;

// include description files for other libraries used (if any)

// Constructor /////////////////////////////////////////////////////////////////
// Function that handles the creation and setup of instances

ThingSpeakWebhooks::ThingSpeakWebhooks()
{
  // initialize this instance's variables


  // do whatever is required to initialize the library

}

// Public Methods //////////////////////////////////////////////////////////////
// Functions available in Wiring sketches, this library, and other libraries

//Webhook
// {
//     "event": "TSBulkWriteCSV",
//     "responseTopic": "{{PARTICLE_DEVICE_ID}}/hook-response/TSBulkWriteCSV",
//     "url": "https://api.thingspeak.com/channels/{{c}}/bulk_update.csv",
//     "requestType": "POST",
//     "noDefaults": true,
//     "rejectUnauthorized": true,
//     "responseTemplate": "{{success}}",
//     "headers": {
//         "Content-Type": "application/x-www-form-urlencoded"
//     },
//     "form": {
//         "write_api_key": "{{k}}",
//         "time_format": "{{t}}",
//         "updates": "{{d}}"
//     }
// }
void ThingSpeakWebhooks::TSBulkWriteCSV(String channel, String api_key, String time_format, String csvData)
{

  //String urlEncodedData = urlencode(csvData);
  //String data = "{\"c\":\"" + channel + "\",\"k\":\"" + api_key + "\",\"t\":\"" + time_format + "\",\"d\":\"" + urlEncodedData + "\"}";
String data = "{\"c\":\"" + channel + "\",\"k\":\"" + api_key + "\",\"t\":\"" + time_format + "\",\"d\":\"" + csvData + "\"}";
  //  String data = urlencode(String );
  // #ifdef DEBUG
  // Serial.println(data);
  // Serial.println(data.length());
  // #endif

  Particle.publish("TSBulkWriteCSV", data, PRIVATE);

  }

  // Private Methods /////////////////////////////////////////////////////////////
  // Functions only available to other functions in this library
  String ThingSpeakWebhooks::urlencode(String str)
  {
      String encodedString="";
      char c;
      char code0;
      char code1;
      char code2;
      for (int i =0; i < str.length(); i++){
        c=str.charAt(i);
        if (c == ' '){
          encodedString+= '+';
        } else if (isalnum(c)){
          encodedString+=c;
        } else{
          code1=(c & 0xf)+'0';
          if ((c & 0xf) >9){
              code1=(c & 0xf) - 10 + 'A';
          }
          c=(c>>4)&0xf;
          code0=c+'0';
          if (c > 9){
              code0=c - 10 + 'A';
          }
          code2='\0';
          encodedString+='%';
          encodedString+=code0;
          encodedString+=code1;
          //encodedString+=code2;
        }
      }
      return encodedString;

}

// NOT fully tested - note that a callback is needed to retrieve the channel
// number and keys.
// This function takes equal length char arrays and creates a channel.
// "end" must be the last value in the array.
// The Particle.publish limits the data to 256 bytes so the array index is returned
// where the createTSChan function left off
// so that additional fields can be updated using updateTSChan.
// A returnIndex of -1 means that there are nothing more to update.
// Although a POST would be easier here this method of using webhooks keeps the
// ThingSpeak User API Key secure by not exposing it in the device.
// Returns true if it did not exit prematurely but the real test is if a valid
// webhook callback occurs and returns the channel id and keys.
// {
//     "event": "TSCreateChannel",
//     "responseTopic": "{{PARTICLE_DEVICE_ID}}/hook-response/TSCreateChannel",
//     "url": "https://api.thingspeak.com/channels.json",
//     "requestType": "POST",
//     "noDefaults": true,
//     "rejectUnauthorized": true,
//     "responseTemplate": "{\"i\":{{id}},\"w\":{{api_keys.0.api_key}}\", \"r\":\"{{api_keys.1.api_key}}\"}",
//     "headers": {
//         "Content-Type": "application/x-www-form-urlencoded"
//     },
//     "form": {
//         "api_key": "your_user_api_key",
//         "description": "{{d}}",
//         "elevation": "{{e}}",
//         "field1": "{{1}}",
//         "field2": "{{2}}",
//         "field3": "{{3}}",
//         "field4": "{{4}}",
//         "field5": "{{5}}",
//         "field6": "{{6}}",
//         "field7": "{{7}}",
//         "field8": "{{8}}",
//         "latitude": "{{a}}",
//         "longitude": "{{o}}",
//         "name": "{{n}}",
//         "public_flag": "{{f}}",
//         "tags": "{{t}}",
//         "url": "{{u}}",
//         "metadata": "{{m}}"
//     }
// }
boolean ThingSpeakWebhooks::TSCreateChan(char const* keys[], char const* values[], int& returnIndex)
{
    char pub[256]; // Size limited by Particle.publish()
    strcpy(pub, "{");
    boolean valid = false;
    uint8_t i=0;
    int len = 1;
    boolean arrayEnd = false;
    boolean done = false;
    unsigned long startTime = millis();
    unsigned long timeOut = 5000;
    while (!done && ((millis() - startTime) < timeOut)) // && (strcmp(labels[i], "end")!=0)
    {
        switch (state) {
            case START:
                if (strcmp(values[i], "end")==0)
                {
                    // Got to the end with no first value
                    valid = false;
                    done = true;
                }
                else
                {
                    if (strlen(values[i])>0)
                    {
                        len = len + strlen(keys[i]) + strlen(values[i]) + 6;
                        if (len<=256)
                        {
                            // Seems lighter to just build the json string rather than
                            // using ArduinoJson library
                            strcat(pub, "\"");
                            strcat(pub, keys[i]);
                            strcat(pub, "\":\"");
                            strcat(pub, values[i]);
                            strcat(pub, "\"");
                // pub = pub + "\"" + names[i] + "\":\"" + values[i] + "\"";
                            state = ADD_NEXT;
                        }
                        else
                        {
                            // First record is longer than it can be
                            valid = false;
                            done = true;
                        }
                    }
                    i++;
                }
            break;

                state = ADD_NEXT;
            break;

            case ADD_NEXT:
                if (strcmp(values[i], "end")==0)
                {
                    // Got to the end
                    arrayEnd = true;
                    state = CREATE_CHANNEL;
                    break;
                }
                else
                {
                    if (strlen(values[i])>0)
                    {
                        len = len + strlen(keys[i]) + strlen(values[i]) + 6;
                        if (len<=256)
                        {
                            strcat(pub, ",\"");
                            strcat(pub, keys[i]);
                            strcat(pub, "\":\"");
                            strcat(pub, values[i]);
                            strcat(pub, "\"");
                            // pub = pub + ",\"" + names[i] + "\":\"" + values[i] + "\"";
                        }
                        else
                        {

                           // Buffer is full
                           state = CREATE_CHANNEL;
                           break;
                        }
                    }
                }

                i++;
            break;

            case CREATE_CHANNEL:
                //Close json
                strcat(pub, "}");
                Particle.publish("TSCreateChannel",pub);
                valid = true;
                //Serial.println(pub);
                if (arrayEnd)
                {
                    returnIndex = -1;
                }
                else
                {
                    returnIndex = (int)i;
                }
                done = true;
            break;

        }
    }

    return valid;
}

boolean ThingSpeakWebhooks::updateTSChan(char const* channel, char const* values[], char const* labels[], int& arrayIndex)
{
    char pub[256];
    unsigned long startTime = millis();
    unsigned long timeOut = 13000;
    boolean valid = false;
    boolean done = false;
    int count = 0;
// delay(1000);
    unsigned long beginTime = millis();
    delay(1);
    // pointer to array values
    uint8_t i=(uint8_t)arrayIndex;
    int len;

    while (!done && ((millis() - startTime) < timeOut))
    {

        if (strcmp(values[i], "end")==0)
        {
            // Done
            done = true;
            break;
        }
        else
        {

            if (strlen(values[i])>0)
            {
                //Serial.println(millis()-beginTime);

                len = strlen(labels[i]) + strlen(values[i]) + strlen(channel) + 15;
                if (len<=256)
                {
                    strcpy(pub, "{\"n\":\"");
                    strcat(pub, labels[i]);
                    strcat(pub, "\",\"d\":\"");
                    strcat(pub, values[i]);
                    strcat(pub, "\",\"c\":\"");
                    strcat(pub, channel);
                    strcat(pub, "\"}");
                    // Need to rate limit to 4 every 4 seconds
                    // if (count >= 4)
                    // {
                    //     delay(4001);
                    //     // simple blocking delay
                    //     count = 0;
                    // }
                    delay(1001);
                    // {"c":"","n":"","d":"","c":""} = 29
                    Particle.publish("TSWriteOneSetting", pub, PRIVATE);
                    valid = true;
                    //Serial.println(pub);
                }

                count++;
            }
        }
        i++;
    }
    return valid;
}

void ThingSpeakWebhooks::TSWriteOneSetting(int channelNum, String value, String label)
{
  String pub = "{\"n\":\"" + label + "\",\"d\":\"" + value + "\",\"c\":\"" + String(channelNum) + "\"}";
  Particle.publish("TSWriteOneSetting", pub, PRIVATE);
}
