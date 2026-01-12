#ifndef FIXED_QUANTITIES_HPP
#define FIXED_QUANTITIES_HPP
#include "wateringlogic.hpp"

class FixedQuantity: public WateringLogic
{
    public:
        FixedQuantity(
            const char* id,
            uint32_t position,
            IntegerProperty& feed_quantity, 
            IntegerProperty& timeout,
            Feeder& act_feeder, 
            Debug& output
        );
        void begin();
        void loop();
    
    private:

};

FixedQuantity::FixedQuantity(
    const char* id,
    uint32_t position,
    IntegerProperty& feed_quantity, 
    IntegerProperty& timeout,
    Feeder& act_feeder, 
    Debug& output
):
    WateringLogic(id, position, feed_quantity, timeout, act_feeder, output)
{}

void FixedQuantity::begin()
{
    WateringLogic::begin();
} 

void FixedQuantity::loop()
{
    if(ready && ((millis() - last_state_change) >= uint32_t(timeout->get()) * 1000ul))
    {
        last_state_change = millis();
        uint32_t quantity = uint32_t(feed_quantity->get());
        debug.print("[Watering] ", id, ": watering fixed quantity: ", quantity);
        act_feeder.start_feed(position, quantity);
    }
}

#endif