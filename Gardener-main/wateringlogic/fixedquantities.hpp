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
            Feeder& act_feeder
        );
        void begin(Debug& debugger = emptydebug);
        void execute();
    
    private:

};

FixedQuantity::FixedQuantity(
    const char* id,
    uint32_t position,
    IntegerProperty& feed_quantity, 
    IntegerProperty& timeout,
    Feeder& act_feeder
):
    WateringLogic(id, position, feed_quantity, timeout, act_feeder)
{}

void FixedQuantity::begin(Debug& debugger)
{
    WateringLogic::begin(debugger);
} 

void FixedQuantity::execute()
{
    uint32_t quantity = uint32_t(feed_quantity.get());
    debug->print("[Watering] ", id, ": watering fixed quantity: ", quantity);
    act_feeder.start_feed(position, quantity);
    
}

#endif