#ifndef WATERING_LOGIC_HPP
#define WATERING_LOGIC_HPP
#include "Property.hpp"
#include "feeder.hpp"
#include "debug.hpp"

/* Base class for all derived classes implementing a specific watering strategy. */
class WateringLogic
{
    public:
        virtual void begin();
        virtual void loop();
    protected:
        WateringLogic(
            const char* id,
            uint32_t position,
            IntegerProperty& feed_quantity, 
            IntegerProperty& timeout,
            Feeder& act_feeder, 
            Debug& output
        );
        const char* id;
        Feeder& act_feeder;
        Debug& debug;
        uint32_t position;
        IntegerProperty* feed_quantity;
        IntegerProperty* timeout;
        uint32_t last_state_change = 0;
        bool ready = false;
    private:
};

WateringLogic::WateringLogic(
    const char* id,
    uint32_t position,
    IntegerProperty& feed_quantity, 
    IntegerProperty& timeout,
    Feeder& feeder, 
    Debug& output = emptydebug
):
    id(id),    
    position(position),
    feed_quantity(&feed_quantity),
    timeout(&timeout),
    act_feeder(feeder),
    debug(output)
{}

void WateringLogic::begin()
{
    ready = true;
    if(feed_quantity == nullptr || timeout == nullptr)
    {
        // ERROR properties not set.
        ready = false;
    }
}

void WateringLogic::loop()
{

}
 

#endif