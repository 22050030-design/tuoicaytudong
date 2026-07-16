#include "sensor_manager.h"

void SensorManager::begin() {
    _dht.begin();
    pinMode(PIN_SOIL_MOISTURE, INPUT);
    pinMode(PIN_WATER_LEVEL, INPUT);
}

float SensorManager::_filteredSoil(float raw) {
    _soilBuffer[_filterIndex] = raw;
    _filterIndex = (_filterIndex + 1) % FILTER_DEPTH;
    if (_filterIndex == 0) _filterFull = true;

    int count = _filterFull ? FILTER_DEPTH : _filterIndex;
    float sum = 0;
    for (int i = 0; i < count; i++) sum += _soilBuffer[i];
    return sum / count;
}

bool SensorManager::_validateReading(const SensorData& d) {
    if (d.temperature < -10 || d.temperature > 60) {
        _error = true;
        _errorMsg = "DHT temp err";
        return false;
    }
    if (d.airHumidity < 0 || d.airHumidity > 100) {
        _error = true;
        _errorMsg = "DHT hum err";
        return false;
    }
    _error = false;
    _errorMsg = "";
    return true;
}

SensorData SensorManager::read() {
    SensorData d;

    // ── Soil moisture ──
    d.soilRaw = analogRead(PIN_SOIL_MOISTURE);
    float avgRaw = _filteredSoil((float)d.soilRaw);
    d.soilMoisture = map((long)avgRaw, SOIL_ADC_DRY, SOIL_ADC_WET,
                         SOIL_MOISTURE_MIN, SOIL_MOISTURE_MAX);
    d.soilMoisture = constrain(d.soilMoisture, 0.0f, 100.0f);

    // ── Water level ──
    int waterRaw = analogRead(PIN_WATER_LEVEL);
    d.waterAvailable = (waterRaw > WATER_LEVEL_DRY);

    // ── DHT22 ──
    d.temperature = _dht.readTemperature();
    d.airHumidity  = _dht.readHumidity();

    // ── Validate ──
    _validateReading(d);

    d.timestamp = millis();
    return d;
}
