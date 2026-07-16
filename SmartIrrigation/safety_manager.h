#ifndef SAFETY_MANAGER_H
#define SAFETY_MANAGER_H

#include <Arduino.h>
#include "config.h"
#include "pump_controller.h"

class SafetyManager {
public:
    void begin(PumpController* pump);

    void check(const SensorData& data);

    bool isAlarmActive() const { return _alarmActive; }
    const char* getAlarmMessage() const { return _alarmMsg; }
    bool isWaterEmpty() const { return _waterEmpty; }
    bool isSensorFault() const { return _sensorFault; }
    bool isPumpTimeout() const { return _pumpTimeout; }

    void clearAlarm();

private:
    PumpController* _pump = nullptr;
    bool        _alarmActive = false;
    bool        _waterEmpty  = false;
    bool        _sensorFault = false;
    bool        _pumpTimeout = false;
    const char* _alarmMsg    = "";
    uint32_t    _alarmTime   = 0;
};

#endif // SAFETY_MANAGER_H
