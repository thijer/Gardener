#ifndef MOISTURE_LEVELS_HPP
#define MOISTURE_LEVELS_HPP
#include <vector>
#include <initializer_list>
#include <utility>
#include "wateringlogic.hpp"

// typedef std::pair<IntegerProperty*, double> std::pair<IntegerProperty*, double>;

/* 
Watering logic that uses moisture sensors to determine when to water the plants,
*/
class MoistureLevels: public WateringLogic
{
    public:
        // Construct a watering group.
        /// @brief 
        /// @param id A string identifying this watering group.
        /// @param position The position on the feeder watering carriage this group is connected to.
        /// @param feed_quantity (s) The amount of seconds the feeder valve needs to be open for a single watering session.
        /// @param timeout (s) Minimum time required to let the supplied water to sink into the ground before checking the moisture sensors again.
        /// @param moisture_threshold (Ohm) The sensor resistance above which water needs to be supplied to the plants.
        /// @param act_feeder The `Feeder` object that will water the plants.
        /// @param output Optionally print debug messages to this object.
        MoistureLevels(
            const char* id,
            uint32_t position,
            IntegerProperty& feed_quantity, 
            IntegerProperty& timeout,
            IntegerProperty& moisture_threshold,
            Feeder& act_feeder, 
            Debug& output
        );

        /// @brief Add a single moisture sensor and its associated weight to the group of sensors.
        /// @param sensor A pointer to an `IntegerProperty` that contains the soil moisture measurements.
        /// @param weight The weight of this moisture value to the weighted mean of all the moisture measurements.
        void add_sensor(IntegerProperty* sensor, double weight);

        /// @brief Add a set of moisture sensors and their weights to this watering group. Any existing sensors will be detached from this group.
        /// @param sensors An initializer list containing pairs of moisture sensors and their weights.
        void add_sensors(std::initializer_list<std::pair<IntegerProperty*, double>> sensors);
        void begin();
        void loop();
    
    private:
        
        std::vector<std::pair<IntegerProperty*, double>> weighted_sensors;
        IntegerProperty* moisture_threshold;

        
        double normalizer = 0.0;
};

MoistureLevels::MoistureLevels(
    const char* id,
    uint32_t position,
    IntegerProperty& feed_quantity, 
    IntegerProperty& timeout,
    IntegerProperty& moisture_threshold,
    Feeder& act_feeder, 
    Debug& output
):
    WateringLogic(id, position, feed_quantity, timeout, act_feeder, output),
    moisture_threshold(&moisture_threshold)
{}

void MoistureLevels::add_sensor(IntegerProperty* sensor, double weight)
{
    // Add the sensor to the existing set.
    weighted_sensors.push_back(std::pair<IntegerProperty*, double>(sensor, weight));
}

void MoistureLevels::add_sensors(std::initializer_list<std::pair<IntegerProperty*, double>> sensors)
{
    // Create a set of sensors from the supplied set.
    weighted_sensors = sensors;
}

void MoistureLevels::begin()
{
    WateringLogic::begin();
    // Check for nullpointers
    if(feed_quantity == nullptr || timeout == nullptr || moisture_threshold == nullptr)
    {
        // ERROR properties not set.
        ready = false;
    }
    normalizer = 0.0;
    for(std::pair<IntegerProperty*, double>& sens : weighted_sensors)
    {
        if(sens.first == nullptr)
        {
            // ERROR one of the sensors is not set.
            ready = false;
        }
        // Normalizing factor is the sum of all sensor weights.
        normalizer += sens.second;
    }
} 

void MoistureLevels::loop()
{
    // Check soil moisture and water if necessary once per `timeout` seconds.
    if(ready && ((millis() - last_state_change) >= timeout->get()))
    {
        last_state_change = millis();
        uint32_t weighted_sum = 0;
        for(std::pair<IntegerProperty*, double>& sens : weighted_sensors)
        {
            // Obtain soil moisture
            uint32_t measurement = sens.first->get();
            weighted_sum += uint32_t(double(measurement) * sens.second / normalizer);
            debug.print("[MoistureLevels] ", id, " measurement: ", measurement, ", weight: ", sens.second);
        }
        uint32_t threshold = uint32_t(moisture_threshold->get());
        debug.print("[MoistureLevels] ", id, " weighted total: ", weighted_sum, ", threshold: ", threshold);
        // Compare weighted soil moisture values to threshold.
        if(weighted_sum > uint32_t(moisture_threshold->get()))
        {
            // And water the plants.
            act_feeder.start_feed(position, uint32_t(feed_quantity->get()));
        }
    }
}

#endif