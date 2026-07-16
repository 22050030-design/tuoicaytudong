#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "config.h"

class DisplayManager {
public:
    void begin();
    void update(const SensorData& data, FSMState state, bool pumpOn,
                bool alarmActive, const char* alarmMsg);

private:
    LiquidCrystal_I2C* _lcd = nullptr;
    uint32_t _lastUpdate = 0;
    uint8_t  _alarmBlink = 0;

    void _updateLCD(const SensorData& data, FSMState state, bool pumpOn,
                    bool alarmActive, const char* alarmMsg);
    void _updateLEDs(FSMState state, bool alarmActive);
    void _updateBuzzer(bool alarmActive);
};

#endif // DISPLAY_MANAGER_H
