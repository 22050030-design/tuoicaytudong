#ifndef PUMP_CONTROLLER_H
#define PUMP_CONTROLLER_H

#include <Arduino.h>
#include "config.h"

class PumpController {
public:
    void begin();

    void pumpOn();
    void pumpOff();
    bool isPumping() const { return _pumpActive; }

    uint32_t getStartTime() const { return _startTime; }
    uint32_t getRunDuration() const;

    void reset();

private:
    bool     _pumpActive = false;
    uint32_t _startTime  = 0;
};

#endif // PUMP_CONTROLLER_H
