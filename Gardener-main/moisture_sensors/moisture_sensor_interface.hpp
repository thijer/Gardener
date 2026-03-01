#ifndef MOISTURE_SENSOR_INTERFACE_HPP
#define MOISTURE_SENSOR_INTERFACE_HPP
#include "property.hpp"
#include "config.h"
#include "debug.hpp"
#include "moisture_sensors/moisture_sensor_base.hpp"

/// @brief An interface that controls the HC4067 multiplexer to which up to 12 soil
/// moisture sensors can be connected. 
class MoistureSensorInterface
{
    public:
        /// @brief 
        /// @param pin_enable Enable pin of the multiplexer.
        /// @param pin_addr0 Address pin 0
        /// @param pin_addr1 Address pin 1
        /// @param pin_addr2 Address pin 2
        /// @param pin_addr3 Address pin 3
        /// @param pin_sens0 Control/measurement pin 0
        /// @param pin_sens1 Control/measurement pin 1
        /// @param sensors List of `MoistureSensor`s to measure 
        /// @param update_interval How often to update the measurements. All sensors are updated at once.
        MoistureSensorInterface(
            uint8_t pin_enable,
            uint8_t pin_addr0,
            uint8_t pin_addr1,
            uint8_t pin_addr2,
            uint8_t pin_addr3,
            uint8_t pin_sens0,
            uint8_t pin_sens1,
            std::initializer_list<MoistureSensorBase*> sensors,
            IntegerProperty& update_interval
        );
        /// @brief Initialize the interface.
        /// @param debugger An optional `Debug` instance.
        void begin(Debug& debugger = emptydebug);
        void loop();
    private:
        /// @brief Get the resistance (Ohm) of the soil moisture sensor connected to the given address.
        /// @param address The address of the moisture sensor.
        /// @return The resistance in Ohms.
        uint32_t get_resistance(uint8_t address);

        const uint8_t pin_enable;
        const uint8_t pin_addr0;
        const uint8_t pin_addr1;
        const uint8_t pin_addr2;
        const uint8_t pin_addr3;
        const uint8_t pin_sens0;
        const uint8_t pin_sens1;
        
        std::vector<MoistureSensorBase*> sensors;
        IntegerProperty& update_interval;
        Debug* debug;
        uint8_t iterator = 0;
        uint32_t last_check = 0;
        uint32_t measurement_ts = 0;
        bool error = false;
};

MoistureSensorInterface::MoistureSensorInterface(
    uint8_t pin_enable,
    uint8_t pin_addr0,
    uint8_t pin_addr1,
    uint8_t pin_addr2,
    uint8_t pin_addr3,
    uint8_t pin_sens0,
    uint8_t pin_sens1,
    std::initializer_list<MoistureSensorBase*> sensors,
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

void MoistureSensorInterface::begin(Debug& debugger)
{
    debug = &debugger;
    pinMode(pin_addr0, OUTPUT); digitalWrite(pin_addr0, 0);
    pinMode(pin_addr1, OUTPUT); digitalWrite(pin_addr1, 0);
    pinMode(pin_addr2, OUTPUT); digitalWrite(pin_addr2, 0);
    pinMode(pin_addr3, OUTPUT); digitalWrite(pin_addr3, 0);
    // Disable multiplexer.
    pinMode(pin_enable, OUTPUT); digitalWrite(pin_enable, 1);
    analogReadResolution(MS_ADC_RESOLUTION);
}

void MoistureSensorInterface::loop()
{
    // This loop will only process one sensor at each call, to prevent blocking other loops for too long.
    if((millis() - last_check) >= (uint32_t(update_interval.get()) * 1000ul))
    {
        // Record starting time.
        if(iterator == 0)
        {
            measurement_ts = millis();
            debug->print("[Moisture] Starting measuring.");
        }
        bool searching_sensor = true;
        // Loop until the next enabled sensor
        while(searching_sensor)
        {
            MoistureSensorBase* sensor = sensors.at(iterator);
            if(sensor->is_enabled())
            {
                searching_sensor = false;
                uint32_t resistance = get_resistance(sensor->get_address());
                sensor->set_moisture(resistance);
                debug->print("[Moisture] Sensor: ", sensor->get_name(), ",", int(sensor->get_address()), ": ", resistance);
            }
            // Increase iterator.
            iterator = (iterator + 1) % sensors.size();
            // Set last_check only when all sensors have been read.
            if(iterator == 0)
            {
                last_check = measurement_ts;
                searching_sensor = false;
            }
        }
    }
}

uint32_t MoistureSensorInterface::get_resistance(uint8_t address)
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
        
        delayMicroseconds(950);
        
        // reverse polarity
        pinMode(pin_sens0, INPUT);
        pinMode(pin_sens1, OUTPUT);
        
        digitalWrite(pin_sens1, 1);
        delayMicroseconds(40);
        m = analogRead(pin_sens0);
        total += m;
        delayMicroseconds(10);
        digitalWrite(pin_sens1, 0);
        
        delayMicroseconds(950);
    }
    
    digitalWrite(pin_enable, 1);
    
    uint32_t average = total / MS_READINGS;
    uint32_t resistance = (MS_RANGE_ADC * MS_FIXED_RESISTOR) / (average - MS_RANGE_LOWER_ADC) - MS_FIXED_RESISTOR;

    // debug->print("[Moisture] raw: ", average, " resistance (ohm): ", resistance);

    return resistance;
}


#endif