#ifndef WATERINGRULE_HPP
#define WATERINGRULE_HPP
#include "ArduinoJson.h"
#include "../src/TinyExpr/tinyexpr.h"
#include "Rule.hpp"
#include "../Feeder/Feeder.hpp"

/// @brief Construct a rule that governs the watering of a specific area of the greenhouse.
class WateringRule: public Rule
{
    friend class RuleEngine;

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
            Feeder& feeder,
            te_parser baseparser = te_parser(),
            uint32_t last_eval = 0
        ):
            Rule(name, expression, eval_interval, enabled, baseparser, last_eval),
            feeder_address(feeder_address),
            act_feeder(feeder)
        {}

        void set_from_json(JsonVariant var)
        {
            if(!var.is<JsonObject>()) return;
            JsonObject params = var.as<JsonObject>();

            if(!(
                params["expression"].is<const char*>() &&
                params["eval_interval"].is<uint32_t>() &&
                params["enabled"].is<bool>() &&
                params["address"].is<uint32_t>()
            )){
                // JsonObject does not contain correct keys.
                return;
            }
            expression = params["expression"].as<std::string>();
            eval_interval = params["eval_interval"].as<uint32_t>();
            enabled = params["enabled"].as<bool>();
            feeder_address = params["address"].as<uint32_t>();
            compiled = false;
            return;
        }
        
        void update()
        {
            uint32_t quantity = uint32_t(evaluate());
            if(quantity > 0) act_feeder.start_feed(feeder_address, quantity);
        }
        
    private:
        /// @brief The address of the feeder to which the plants will be watered with the quantity calculated by the expression.
        uint32_t feeder_address;
        Feeder& act_feeder;
};

#endif