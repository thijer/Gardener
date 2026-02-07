#ifndef MOISTURE_SENSOR_BASE_HPP
#define MOISTURE_SENSOR_BASE_HPP
#include "property.hpp"

/// @brief Base class for MoistureSensors, mainly so that MoistureSensorInterface can work with both ThingDevice moisture sensors and regular ones.
class MoistureSensorBase
{
    protected:
        MoistureSensorBase(const char* sensorname, uint8_t address):
            name(sensorname),
            address(address)
        {}
    public:

        const char* get_name() { return name; }
        uint8_t get_address() { return address; }
        virtual bool is_enabled() = 0;
        virtual IntegerProperty* get_moisture() = 0;

    private:
        uint8_t address;
        const char* name;
};

#endif