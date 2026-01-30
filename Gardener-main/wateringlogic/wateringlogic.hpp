#ifndef WATERING_LOGIC_HPP
#define WATERING_LOGIC_HPP
#include "Property.hpp"
#include "feeder.hpp"
#include "debug.hpp"

/* Base class for all derived classes implementing a specific watering strategy. */
class WateringLogic
{
    public:
        virtual void begin(Debug& debugger = emptydebug);
        void loop();
        virtual void execute();
        void enable()  { enabled = true; }
        void disable() { enabled = false; }
    
    protected:
        WateringLogic(
            const char* id,
            uint32_t position,
            IntegerProperty& feed_quantity, 
            IntegerProperty& timeout,
            Feeder& act_feeder
        );
        const char* id;
        Feeder& act_feeder;
        Debug* debug;
        uint32_t position;
        IntegerProperty* feed_quantity;
        IntegerProperty* timeout;
        uint32_t last_state_change = 0;
        bool ready = false;
        bool enabled = true;
    private:
};

WateringLogic::WateringLogic(
    const char* id,
    uint32_t position,
    IntegerProperty& feed_quantity, 
    IntegerProperty& timeout,
    Feeder& feeder
):
    id(id),    
    position(position),
    feed_quantity(&feed_quantity),
    timeout(&timeout),
    act_feeder(feeder)
{}

void WateringLogic::begin(Debug& debugger)
{
    debug = &debugger;
    ready = true;
    if(feed_quantity == nullptr || timeout == nullptr)
    {
        // ERROR properties not set.
        ready = false;
    }
}

void WateringLogic::loop()
{
    if(enabled && ready && ((millis() - last_state_change) >= uint32_t(timeout->get()) * 1000ul))
    {
        last_state_change = millis();
        execute();
    }
}

void WateringLogic::execute()
{
    
}

#endif