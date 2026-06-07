#ifndef MOISTURE_SENSOR_HPP
#define MOISTURE_SENSOR_HPP
#include "property.hpp"
#include "MoistureSensor/MoistureSensorBase.hpp"

/// @brief A virtual device that ties moisture measurement storage to the address of the physical sensor.
class MoistureSensor: public MoistureSensorBase
{
    public:
        /// @brief Construct a moisture sensor device.
        /// @param name The name under which the soil moisture measurements are stored.
        /// @param address The address of the physical port the sensor is connected to.
        /// @param alpha A calibrated factor used to calculate the temperature correction for this sensor.
        /// @param enabled If the sensor should be enabled by default.
        MoistureSensor(const char* name, uint8_t address, double alpha, bool enabled = true):
            MoistureSensorBase(name, address, alpha, enabled)
        {}
        
    private:
};

#endif