#ifndef TEMP_SENSOR_HPP
#define TEMP_SENSOR_HPP
#include "config.h"
#include <DHT_U.h>
#include <DHT.h>
#include "property.hpp"

class TempHumSensor
{
    public:
        TempHumSensor(uint pin, FloatProperty* temp, FloatProperty* hum);
        void property_reading_interval(IntProperty* p) { reading_interval = p; }
        void loop();
        // void apply_setting(settings* config);
        // double temp;
        // double hum;
        
    private:
        FloatProperty* temp;
        FloatProperty* hum;
        IntProperty* reading_interval;
        
        bool desired_state = false;

        uint last_update = 0;

        DHT sensor;
};

TempHumSensor::TempHumSensor(uint pin, FloatProperty* temp, FloatProperty* hum):
    sensor(pin, DHT22),
    temp(temp),
    hum(hum)
{
    sensor.begin();
}

void TempHumSensor::loop()
{
    if(reading_interval != nullptr)
    {
        if(millis() - last_update >= reading_interval->get())
        {
            last_update = millis();
            temp->set(double(sensor.readTemperature()));
            hum->set(double(sensor.readHumidity()));
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