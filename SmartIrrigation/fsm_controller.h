#ifndef FSM_CONTROLLER_H
#define FSM_CONTROLLER_H

#include <Arduino.h>
#include "config.h"
#include "pump_controller.h"

class FsmController {
public:
    void begin(PumpController* pump);

    void update(const SensorData& data, bool cmdWater, bool cmdStop);

    FSMState getState() const { return _state; }
    const char* getStateName() const;

    void enterAlarm();
    void clearAlarm();
    void startManual();
    void stopManual();

private:
    PumpController* _pump = nullptr;
    FSMState        _state = STATE_INIT;
    FSMState        _prevState = STATE_INIT;
    uint32_t        _stateEntryTime = 0;

    // Schedule
    bool    _scheduleEnabled = false;
    uint8_t _scheduleHour1 = 6;
    uint8_t _scheduleMin1  = 0;
    uint8_t _scheduleHour2 = 17;
    uint8_t _scheduleMin2  = 0;

    void _changeState(FSMState next);
    void _onEnterState(FSMState s);
    void _handleInit(const SensorData& data);
    void _handleMonitoring(const SensorData& data, bool cmdWater, bool cmdStop);
    void _handleAutoWatering(const SensorData& data);
    void _handleManualWatering(const SensorData& data, bool cmdStop);
    void _handleCooldown();
    void _handleAlarm(bool cmdStop);
    void _handleOffline(const SensorData& data, bool cmdWater);
    bool _isScheduleTime();
};

#endif // FSM_CONTROLLER_H
