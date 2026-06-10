#ifndef MOISTURE_SENSOR_DEVICE_HPP
#define MOISTURE_SENSOR_DEVICE_HPP
#include "property.hpp"
#include "propertystore.hpp"
#include "ThingDevice.hpp"
#include "MoistureSensor/MoistureSensorBase.hpp"

/// @brief A virtual device that reports the value of the soil moisture sensor connected 
/// to the given address of the `MoistureSensorArray` to Thingsboard.
class MoistureSensor: public ThingDevice, public MoistureSensorBase
{
    public:
        /// @brief Set up a moisture sensor 
        /// @param name The name under which it appears in Thingsboard.
        /// @param address The address (connector) to which the physical moisture sensor is connected.
        /// @param alpha A calibrated factor used to calculate the temperature correction for this sensor.
        /// @param enabled If the sensor should be enabled by default.
        MoistureSensor(const char* name, uint8_t address, double alpha, bool enabled = true):
            ThingDevice(name, "Gardener-moisture-sensor"),  // Initialize a `ThingDevice` with the given name.
            MoistureSensorBase(name, address, alpha, enabled),
            moisture_tb("moisture"),                           // moisture measurements will appear in Thingsboard as "moisture" under telemetry.
            telemetry({&moisture_tb})                              // Add the property to a `TelemetryStore` that in turn will be used by the `ThingDevice`.
        {}

        void begin()
        {
            // Add the telemetry property to the `ThingDevice`.
            ThingDevice::add_telemetry(telemetry);
            ThingDevice::add_shared_attributes(attributes);
            MoistureSensorBase::begin();
        }

        void loop()
        {
            ThingDevice::loop();
            MoistureSensorBase::loop();
        }
        void set_moisture(int32_t moist) { moisture_tb.set(moist); moisture.set(moist); }
        
    private:
        IntegerProperty moisture_tb;        // Used by thingboard with the name "moisture"
        PropertyStore<1> telemetry;
};


#endif