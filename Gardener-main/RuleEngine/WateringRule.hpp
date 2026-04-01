#ifndef WATERINGRULE_HPP
#define WATERINGRULE_HPP
#include "../src/TinyExpr/tinyexpr.h"
#include "PropertyRule.hpp"

/// @brief Construct a rule that governs the watering of a specific area of the greenhouse.
class WateringRule: public PropertyRule<int32_t>
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
            te_parser baseparser = te_parser(),
            uint32_t last_eval = 0
        ):
            PropertyRule<int32_t>(name, expression, eval_interval, enabled, baseparser, last_eval),
            feeder_address(feeder_address)
        {}
    private:
        /// @brief The address of the feeder to which the plants will be watered with the quantity calculated by the expression.
        uint32_t feeder_address;
};

#endif