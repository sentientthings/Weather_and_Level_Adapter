#ifndef IoTNodePower_h
#define IoTNodePower_h

#include "application.h"

enum powerName {INT5V, INT12V, EXT3V3, EXT5V, EXT12V};

class IoTNodePower
    {
        public:

        IoTNodePower();

        void begin();
        void setPowerON(powerName pwrName, bool state);
        void tickleWatchdog();
        bool isLiPoPowered();
        bool is3AAPowered();
        bool isLiPoCharged();
        bool isLiPoCharging();
        float voltage();

        private:
    };

#endif
