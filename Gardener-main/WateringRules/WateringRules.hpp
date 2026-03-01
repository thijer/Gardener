#ifndef WATERINGRULES_HPP
#define WATERINGRULES_HPP
#include "../RuleEngine/RuleEngineBase.hpp"
#include "../feeder.hpp"

/// @brief Construct a rule that governs the watering of a specific area of the greenhouse.
class WateringRule: public Rule
{
    friend class WateringRuleEngine;

    public:
        /// @brief Construct the rule.
        /// @param name The name of this rule.
        /// @param expression The expression to evaluate
        /// @param eval_interval The interval between evaluations.
        /// @param feeder_address The address of the feeder.
        /// @param enabled If the expression should be evaluated from startup, if false it will not run until enabled.
        /// @param baseparser A `te_parser` instance from the `RuleEngine` that is already furnaced with the available variables.
        /// @param last_eval (optional) The last time this rule was evaluated, used for when an expression is updated and needs to keep to its schedule.
        WateringRule(
            const char* name, 
            const char* expression, 
            uint32_t eval_interval,
            uint32_t feeder_address,
            bool enabled,
            te_parser baseparser,
            uint32_t last_eval = 0
        ):
            Rule(name, expression, eval_interval, enabled, baseparser, last_eval),
            feeder_address(feeder_address)
        {}
    private:
        /// @brief The address of the feeder to which the plants will be watered with the quantity calculated by the expression.
        uint32_t feeder_address;
};

/// @brief An implementation of `RuleEngine` to run logic that will water areas of the the greenhouse.
class WateringRuleEngine: public RuleEngine
{
    public:
        /// @brief Construct the rule engine.
        /// @param feeder A `Feeder` instance that will receive the command to water a specific are of the greenhouse.
        WateringRuleEngine(
            Feeder& feeder
        ):
            RuleEngine(),
            feeder(feeder)
        {}
        ~WateringRuleEngine(){}

        /// @brief Run the engine
        void loop();
        /// @brief Print details about this rule engine and its rules to `debug`.
        void print();

    protected:
        /// @brief Process a new rule or update an existing one.
        /// @param pair The rule parameters.
        /// @return true if the rule is processed successfully.
        bool process_rule(JsonPair pair);

    private:
        Feeder& feeder;
        
        /// @brief The set of rules to evaluate.
        std::vector<WateringRule> rules;
};

void WateringRuleEngine::loop()
{
    // Loop through all the rules 
    for(WateringRule& rule : rules)
    {
        // Its time to evaluate one
        if(millis() - rule.last_evaluation >= (rule.eval_interval * 1000ul))
        {
            rule.last_evaluation = millis();
            debug->print("[WateringRuleEngine] evaluating ", rule.get_name());
            te_type res = rule.evaluate();
            debug->print("[WateringRuleEngine] result: ", res);

            // The rule shoudl return 0.0 if the section should not be watered, else 
            // it returns the quantity it should be watered with.
            if(res > 0.0)
            {
                feeder.start_feed(rule.feeder_address, uint32_t(res));
            }
        }
    }
}

bool WateringRuleEngine::process_rule(JsonPair pair)
{
    // JsonPair pair needs to have the following structure:
    // "rule_id": {
    //     "expression": "gerrit + henk == chaos",
    //     "feeder_address": 0,
    //     "eval_interval": 600,
    //     "enabled": true
    // }
    
    const char* rule_name = pair.key().c_str();

    if(!pair.value().is<JsonObject>())
    {
        // Value is not a JsonObject.
        debug->print("[WateringRuleEngine] ERROR: Value is not a JsonObject.");
        return false;
    }
    
    JsonObject params = pair.value();
    if(!(
        params["expression"].is<const char*>() &&
        params["eval_interval"].is<uint32_t>() &&
        params["feeder_address"].is<uint32_t>() &&
        params["enabled"].is<bool>()
    )){
        // JsonObject does not contain correct keys.
        debug->print("[WateringRuleEngine] ERROR: JsonObject does not contain correct keys.");
        return false;
    }  
    
    uint32_t last_evaluation = 0;
    WateringRule* oldrule = nullptr;

    for(WateringRule& rule : rules)
    {
        // Rule already exists.
        if(strcmp(rule.get_name(), rule_name) == 0)
        {
            debug->print("[WateringRuleEngine] Found existing rule with name ", rule_name);
            oldrule = &rule;
            last_evaluation = rule.last_evaluation;
            break;
        }
    }

    // Create rule
    WateringRule newrule(
        rule_name,
        params["expression"].as<const char*>(),
        params["feeder_address"].as<uint32_t>(),
        params["eval_interval"].as<uint32_t>(),
        params["enabled"].as<bool>(),
        base_parser,
        last_evaluation
    );
    
    if(!newrule.compiled)
    {
        // Expression compilation error.
        debug->print("[WateringRuleEngine] ERROR: Expression compilation failed");
        debug->print(newrule.parser.get_last_error_position());
        debug->print(newrule.parser.get_last_error_message().c_str());
        
        return false;
    }
    // Replace old rule
    if(oldrule)
    { 
        debug->print("[WateringRuleEngine] Replacing old rule");
        *oldrule = newrule;
    }
    // Append new rule otherwise.
    else
    {
        debug->print("[WateringRuleEngine] Adding new rule");
        rules.push_back(newrule);
    }
    return true;
}

void WateringRuleEngine::print()
{
    debug->print("[WateringRuleEngine] with ", rules.size(), " rules.");

    for(WateringRule& rule : rules)
    {
        rule.print_to(*debug);
        debug->print("feeder addr: ", rule.feeder_address);
    }
}

#endif