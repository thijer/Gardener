#ifndef MOISTURE_SENSOR_H
#define MOISTURE_SENSOR_H
#include "property.hpp"
#include "config.h"
#include "debug.hpp"

class MoistureSensorArray
{
    public:
        MoistureSensorArray(
            uint8_t pin_enable,
            uint8_t pin_addr0,
            uint8_t pin_addr1,
            uint8_t pin_addr2,
            uint8_t pin_addr3,
            uint8_t pin_sens0,
            uint8_t pin_sens1,
            std::initializer_list<std::pair<IntegerProperty*, uint8_t>> sensors,
            IntegerProperty& update_interval
        );
        void begin(Debug& debugger = emptydebug);
        void loop();
    private:
        uint32_t get_resistance(uint8_t address);

        const uint8_t pin_enable;
        const uint8_t pin_addr0;
        const uint8_t pin_addr1;
        const uint8_t pin_addr2;
        const uint8_t pin_addr3;
        const uint8_t pin_sens0;
        const uint8_t pin_sens1;
        
        std::vector<std::pair<IntegerProperty*, uint8_t>> sensors;
        IntegerProperty& update_interval;
        Debug* debug;
        uint8_t iterator = 0;
        uint32_t last_check = 0;
        uint32_t measurement_ts = 0;
        bool error = false;
};

MoistureSensorArray::MoistureSensorArray(
    uint8_t pin_enable,
    uint8_t pin_addr0,
    uint8_t pin_addr1,
    uint8_t pin_addr2,
    uint8_t pin_addr3,
    uint8_t pin_sens0,
    uint8_t pin_sens1,
    std::initializer_list<std::pair<IntegerProperty*, uint8_t>> sensors,
    IntegerProperty& update_interval
):
    pin_enable(pin_enable),
    pin_addr0(pin_addr0),
    pin_addr1(pin_addr1),
    pin_addr2(pin_addr2),
    pin_addr3(pin_addr3),    
    pin_sens0(pin_sens0),    
    pin_sens1(pin_sens1),    
    sensors(sensors),
    update_interval(update_interval)
{}

void MoistureSensorArray::begin(Debug& debugger)
{
    // error = 
    debug = &debugger;
    pinMode(pin_addr0, OUTPUT); digitalWrite(pin_addr0, 0);
    pinMode(pin_addr1, OUTPUT); digitalWrite(pin_addr1, 0);
    pinMode(pin_addr2, OUTPUT); digitalWrite(pin_addr2, 0);
    pinMode(pin_addr3, OUTPUT); digitalWrite(pin_addr3, 0);
    pinMode(pin_enable, OUTPUT); digitalWrite(pin_enable, 1);
    analogReadResolution(MS_ADC_RESOLUTION);
}

void MoistureSensorArray::loop()
{
    // This loop will only process one sensor at each call, to prevent blocking other loops for too long.
    if((millis() - last_check) >= uint32_t(update_interval.get()))
    {
        // Record starting time.
        if(iterator == 0)
        {
            measurement_ts = millis();
            debug->print("[Moisture] Starting measuring.");
        }
        
        std::pair<IntegerProperty*, uint8_t>& sensor = sensors.at(iterator);
        uint32_t resistance = get_resistance(sensor.second);
        sensor.first->set(int32_t(resistance));
        debug->print("[Moisture] Sensor: ", sensor.first->get_name(), ",", int(sensor.second), ": ", resistance);
        // Increase iterator.
        iterator = (iterator + 1) % sensors.size();

        // Set last_check only when all sensors have been read.
        if(iterator == 0) last_check = measurement_ts;
    }
}

uint32_t MoistureSensorArray::get_resistance(uint8_t address)
{
    // 150 - 3100 mV -> 0 - 4096 -> 151 - 95k5 ohm
    uint32_t total = MS_READINGS;
    
    digitalWrite(pin_addr0, (address >> 0) & 1);
    digitalWrite(pin_addr1, (address >> 1) & 1);
    digitalWrite(pin_addr2, (address >> 2) & 1);
    digitalWrite(pin_addr3, (address >> 3) & 1);
    digitalWrite(pin_enable, 0);
    
    for(uint8_t i = 0; i < MS_READINGS / 2; i++)
    {
        pinMode(pin_sens0, OUTPUT);
        pinMode(pin_sens1, INPUT);
        
        digitalWrite(pin_sens0, 1);
        delayMicroseconds(40);
        uint32_t m = analogRead(pin_sens1);
        total += m;
        delayMicroseconds(10);
        digitalWrite(pin_sens0, 0);
        
        // Serial.println(m);
        delayMicroseconds(950);
        
        // reverse polarization
        pinMode(pin_sens0, INPUT);
        pinMode(pin_sens1, OUTPUT);
        
        digitalWrite(pin_sens1, 1);
        delayMicroseconds(40);
        m = analogRead(pin_sens0);
        total += m;
        delayMicroseconds(10);
        digitalWrite(pin_sens1, 0);
        
        // Serial.println(m);
        delayMicroseconds(950);
    }
    
    digitalWrite(pin_enable, 1);
    
    uint32_t average = total / MS_READINGS;
    uint32_t resistance = (MS_RANGE_ADC * MS_FIXED_RESISTOR) / (average - MS_RANGE_LOWER_ADC) - MS_FIXED_RESISTOR;

    // debug->print("[Moisture] raw: ", average, " resistance (ohm): ", resistance);

    // #ifdef SERIAL_DEBUG
    //     Serial.println("Sample1\tSample2");
    // #endif
    
    // debug->print("Vsupply:\t", vcc, "\tMoisture:\t", average);

    return resistance;
}

#endif