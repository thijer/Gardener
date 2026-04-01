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
        /// @param enabled If the sensor should be enabled by default.
        MoistureSensor(const char* name, uint8_t address, bool enabled = true):
            MoistureSensorBase(name, address),
            moisture(name),
            enabled(name, enabled)
        {}
        // Empty begin and loop functions to maintain compatibility 
        // with the `MoistureSensor` definition in `moisture_sensor_device.hpp`
        void begin() {}
        void loop() {}
        // get access to enabler.
        BooleanProperty*    get_enabler()   { return &enabled; }

        // MoistureSensorBase overrides.
        bool is_enabled() { return enabled.get(); }
        IntegerProperty*    get_moisture()  { return &moisture; }
        void set_moisture(int32_t moist) { moisture.set(moist); }
    private:
        IntegerProperty moisture;
        BooleanProperty enabled;
};

#endif