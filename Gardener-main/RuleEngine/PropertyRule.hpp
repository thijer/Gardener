#ifndef PROPERTYRULE_HPP
#define PROPERTYRULE_HPP
#include <Print.h>
#include "../src/TinyExpr/tinyexpr.h"
#include "property.hpp"
#include "Rule.hpp"


/// @brief The class for `PropertyRuleEngine` rules that store the results of expressions into a `Property` of type `T`.
/// @tparam T The type of the `Property`.
template<typename T>
class PropertyRule: public Rule
{
    friend class PropertyRuleEngine;
    public:
        /// @brief Construct the rule with a provided parser.
        /// @param name The name of this rule.
        /// @param expression The expression to evaluate
        /// @param eval_interval The interval between evaluations.
        /// @param enabled If the expression should be evaluated from startup, if false it will not run until enabled.
        /// @param baseparser A `te_parser` instance from the `RuleEngine` that is already furnaced with the available variables.
        /// @param last_eval (optional) The last time this rule was evaluated, used for when an expression is updated and needs to keep to its schedule.
        PropertyRule(
            const char* name, 
            const char* expression, 
            uint32_t eval_interval,
            bool enabled,
            te_parser baseparser,
            uint32_t last_eval = 0
        ):
            Rule(name, expression, eval_interval, enabled, baseparser, last_eval),
            property(name)
        {}
        /// @brief Construct the rule with a blank parser.
        /// @param name The name of this rule.
        /// @param expression The expression to evaluate
        /// @param eval_interval The interval between evaluations.
        /// @param enabled If the expression should be evaluated from startup, if false it will not run until enabled.
        /// @param last_eval (optional) The last time this rule was evaluated, used for when an expression is updated and needs to keep to its schedule.
        PropertyRule(
            const char* name, 
            const char* expression, 
            uint32_t eval_interval,
            bool enabled = true
        ):
            Rule(name, expression, eval_interval, enabled, te_parser(), 0),
            property(name)
        {}

        /// @brief Evaluate the expression and update the Property with the result.
        void update()
        {
            te_type result = Rule::evaluate();
            property.set(T(result));
        }

        /// @brief Print information about this rule to:
        /// @param sink A `Print` interface, for example `Serial`.
        void print_to(Print& sink)
        {
            Rule::print_to(sink);
            sink.print("value:       "); sink.println(property.get());
        }
        
        Property<T>* get_property() { return &property; }
    private:
        Property<T> property;
        
        
};

#endif