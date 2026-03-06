#ifndef THINGSRULEENGINE_HPP
#define THINGSRULEENGINE_HPP
#include "ThingDevice.hpp"
#include "../RuleEngine/RuleEngineBase.hpp"
#include "property.hpp"
#include "propertystore.hpp"

/// @brief A wrapper to allow a RuleEngine to get its rules from Thingsboard.
class ThingRuleEngine: public ThingDevice
{
    public:
    ThingRuleEngine(RuleEngine& engine, const char* name, const char* type = nullptr, bool enabled = true);
    ~ThingRuleEngine(){}

    void loop();
    protected:
    
    private:
    void process_attributes(JsonObject obj);

    RuleEngine& engine;
    IntegerProperty last_update;
    PropertyStore<1> store;
    
};

ThingRuleEngine::ThingRuleEngine(RuleEngine &engine, const char *name, const char *type, bool enabled):
    ThingDevice(name, type, enabled),
    engine(engine),
    last_update("keep-alive", 0),
    store({&last_update})
{
    ThingDevice::add_client_attributes(store);
}

void ThingRuleEngine::loop()
{
    // Set to 9min (9*60=540) to remain below the default timeout from thingsboard (10min)
    if((millis() / 1000ul) - uint32_t(last_update.get()) >= 540ul)
    {
        last_update.set(millis() / 1000ul);
    }
    ThingDevice::loop();
}

void ThingRuleEngine::process_attributes(JsonObject obj)
{
    engine.process_attributes(obj);
}

#endif