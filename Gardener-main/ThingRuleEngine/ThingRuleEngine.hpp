#ifndef THINGSRULEENGINE_HPP
#define THINGSRULEENGINE_HPP
#include "ThingDevice.hpp"
#include "../RuleEngine/RuleEngineBase.hpp"

/// @brief A wrapper to allow a RuleEngine to get its rules from Thingsboard.
class ThingRuleEngine: public ThingDevice
{
    public:
    ThingRuleEngine(RuleEngine& engine, const char* name, const char* type = nullptr, bool enabled = true);
    ~ThingRuleEngine(){}
    protected:
    
    private:
    void process_attributes(JsonObject obj);

    RuleEngine& engine;
    
};

ThingRuleEngine::ThingRuleEngine(RuleEngine &engine, const char *name, const char *type, bool enabled):
    ThingDevice(name, type, enabled),
    engine(engine)
{}

void ThingRuleEngine::process_attributes(JsonObject obj)
{
    engine.process_attributes(obj);
}

#endif