#ifndef PROPERTYRULE_HPP
#define PROPERTYRULE_HPP
#include <Print.h>
#include "../src/TinyExpr/tinyexpr.h"
#include "property.hpp"
#include "Rule.hpp"

/// @brief The non-templated base class for `PropertyRuleEngine` rules that store the results of expressions into a `Property`.
class BasePropertyRule: public Rule
{
    friend class PropertyRuleEngine;

    public:
        /// @brief A self contained expression of which the result is stored in a self contained `Property`.
        /// @param name The name of the expression and the name of the corresponding `Property`.
        /// @param expression The expression to evaluate.
        /// @param eval_interval The interval between evaluations (s).
        /// @param enabled If the expression should be evaluated from startup, if false it will not run until enabled.
        /// @param baseparser A `te_parser` instance.
        /// @param last_eval (optional) The last time this rule was evaluated, used for when an expression is updated and needs to keep to its schedule.
        BasePropertyRule(
            const char* name, 
            const char* expression, 
            uint32_t eval_interval,
            bool enabled,
            te_parser baseparser,
            uint32_t last_eval = 0
        ):
            Rule(name, expression, eval_interval, enabled, baseparser, last_eval)
        {}
        
        /// @brief Base method that encapsulates the expression evaluation and setting the property to the result.
        virtual void update() = 0;

        /// @brief Print information about this rule to:
        /// @param sink A `Print` interface.
        virtual void print_to(Print& sink) {}
    protected:
        /// @brief Update the parser with a version that likely contains references to more variables.
        /// @param base_parser A `te_parser` instance from the `RuleEngine` that is already furnaced with the available variables.
        void set_parser(te_parser base_parser){ parser = base_parser; }
};

/// @brief The class for `PropertyRuleEngine` rules that store the results of expressions into a `Property` of type `T`.
/// @tparam T The type of the `Property`.
template<typename T>
class PropertyRule: public Property<T>, public BasePropertyRule
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
            BasePropertyRule(name, expression, eval_interval, enabled, baseparser, last_eval),
            Property<T>(name)
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
            BasePropertyRule(name, expression, eval_interval, enabled, te_parser(), 0),
            Property<T>(name)
        {}

        /// @brief Evaluate the expression and update the Property with the result.
        void update()
        {
            te_type result = Rule::evaluate();
            Property<T>::set(T(result));
        }

        /// @brief Print information about this rule to:
        /// @param sink A `Print` interface, for example `Serial`.
        void print_to(Print& sink)
        {
            Rule::print_to(sink);
            sink.print("value:       "); sink.println(Property<T>::get());
        }

        const char* get_name() { return Property<T>::get_name(); }
    private:
        
        
};

#endif