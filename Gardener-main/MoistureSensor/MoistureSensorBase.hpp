#ifndef MOISTURE_SENSOR_BASE_HPP
#define MOISTURE_SENSOR_BASE_HPP
#include "property.hpp"
#include "propertystore.hpp"
#include "PropertyMemory.hpp"
#include "PropertyTextInterface.hpp"
#include "MoistureSensorInterface.hpp"
#include <cmath>

/// @brief Base class for MoistureSensors, mainly so that MoistureSensorInterface can work with both ThingDevice moisture sensors and regular ones.
class MoistureSensorBase
{
    protected:
        friend class MoistureSensorInterface;
        MoistureSensorBase(const char* sensorname, uint8_t address, double alpha, bool enabled):
            name(sensorname),
            prop_address("address", address),
            prop_alpha("alpha", alpha),
            prop_enabled("enabled", enabled),
            moisture(sensorname),
            attributes({&prop_address, &prop_alpha, &prop_enabled}),
            mem(attributes, sensorname),
            interface(attributes)
        {}
        const char* name;
        IntegerProperty moisture;
        IntegerProperty prop_address;
        BooleanProperty prop_enabled;
        RealProperty prop_alpha;

        PropertyStore<3> attributes;
        PropertyMemory mem;
        PropertyTextInterface interface;

    public:
        virtual void begin() { mem.begin(); }
        virtual void loop()  { mem.save(); }
        
        const char* get_name() { return name; }
        uint8_t get_address() { return prop_address.get(); }
        
        bool is_enabled() { return prop_enabled.get(); };
        BooleanProperty* get_enabler() { return &prop_enabled; };
        
        /// @brief Store a temperature corrected measurement.
        /// @param resistance The soil moisture reading (ohm)
        /// @param temp The soil temperature (°C)
        void set_moisture(int32_t resistance, double temp)
        {
            double correction = exp(prop_alpha.get() * (temp - MS_TEMP_REF));
            int32_t corrected = int32_t(round(double(resistance) * correction));
            set_moisture(corrected);
        }
        
        virtual void set_moisture(int32_t moist) { moisture.set(moist); }
        IntegerProperty* get_moisture()  { return &moisture; }
};

#endif