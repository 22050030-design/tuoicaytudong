#ifndef IOT_MANAGER_H
#define IOT_MANAGER_H

#include <Arduino.h>
#include "config.h"

class IoTManager {
public:
    void begin();
    void update(const SensorData& data, FSMState state, bool pumpOn);

    bool isCommandWater() { bool v = _cmdWater; _cmdWater = false; return v; }
    static void setCmdWater(bool val);

private:
    bool     _connected = false;
    bool     _cmdWater  = false;
    uint32_t _lastSend  = 0;

    void _reconnect();
    void _sendData(const SensorData& data, FSMState state, bool pumpOn);
    void _receiveCommands();
};

#endif // IOT_MANAGER_H
