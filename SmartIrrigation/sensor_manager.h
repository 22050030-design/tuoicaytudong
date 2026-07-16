#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <Arduino.h>
#include <DHT.h>
#include "config.h"

class SensorManager {
public:
    void begin();
    SensorData read();
    bool isSensorError() const { return _error; }
    const char* getLastError() const { return _errorMsg; }

private:
    DHT    _dht;
    bool   _error = false;
    const char* _errorMsg = "";

    // Moving-average filter (depth 5)
    static const int FILTER_DEPTH = 5;
    float _soilBuffer[FILTER_DEPTH] = {};
    int   _filterIndex = 0;
    bool  _filterFull  = false;

    float _filteredSoil(float raw);
    bool  _validateReading(const SensorData& d);
};

#endif // SENSOR_MANAGER_H
