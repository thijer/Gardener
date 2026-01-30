#ifndef TEMP_SENSOR_HPP
#define TEMP_SENSOR_HPP
#include "config.h"
#include <DHT_U.h>
#include <DHT.h>
#include "property.hpp"
#include "debug.hpp"

class TempHumSensor
{
    public:
        TempHumSensor(uint8_t pin, RealProperty* temp = nullptr, RealProperty* hum = nullptr);
        void property_reading_interval(IntegerProperty* p) { reading_interval = p; }
        void loop();
        void begin(Debug& debugger = emptydebug);
        bool get_error() { return error; }
        // void apply_setting(settings* config);
        // double temp;
        // double hum;
        
    private:
        bool read();
        RealProperty* temp;
        RealProperty* hum;
        IntegerProperty* reading_interval;
        
        bool desired_state = false;

        uint32_t last_update = 0;
        bool error = false;

        DHT sensor;
        Debug* debug;
};

TempHumSensor::TempHumSensor(uint8_t pin, RealProperty* temp, RealProperty* hum):
    sensor(pin, DHT22),
    temp(temp),
    hum(hum)
{}

bool TempHumSensor::read()
{
    if(temp != nullptr)
    {
        float t = sensor.readTemperature();
        if(isnan(t)) return false;
        temp->set(double(t));
    }
    if(hum  != nullptr)
    {
        float h = sensor.readHumidity();
        if(isnan(h)) return false;
        hum->set(double(h));
    }
    return true;
}

void TempHumSensor::begin(Debug& debugger)
{
    debug = &debugger;
    sensor.begin();
    error = !read();
    last_update = millis();
}

void TempHumSensor::loop()
{
    if(reading_interval != nullptr)
    {
        if(millis() - last_update >= uint32_t(reading_interval->get()))
        {
            last_update = millis();
            error = !read();
        }
    }
}

/* 
    void TempHumSensor::calc_desired_state()
    {
        if(temp_interior > window_open_temp) desired_state = true;
        else if( temp_interior < window_close_temp) desired_state = false;
    }

    void TempHumSensor::apply_setting(settings* config)
    {
        reading_interval = config->get_int("temp_measurement_interval");
        window_open_temp = config->get_double("window_open_temp");
        window_close_temp = config->get_double("window_close_temp");
    }

*/

#endif