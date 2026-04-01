#ifndef RULE_HPP
#define RULE_HPP
#include <string>
#include <Print.h>
#include "../src/TinyExpr/tinyexpr.h"

/// @brief A self-contained expression that will be evaluated at a certain interval. 
class Rule
{
    public:
        /// @brief Construct a rule 
        /// @param name The name of this rule.
        /// @param expression The expression to evaluate
        /// @param eval_interval The interval between evaluations.
        /// @param enabled If the expression should be evaluated from startup, if false it will not run until enabled.
        /// @param baseparser A `te_parser` instance from the `RuleEngine` that is already furnaced with the available variables.
        /// @param last_eval (optional) The last time this rule was evaluated, used for when an expression is updated and needs to keep to its schedule.
        Rule(
            const char* name, 
            const char* expression, 
            uint32_t eval_interval,
            bool enabled,
            te_parser baseparser,
            uint32_t last_eval = 0
        );
        
        /// @brief Print details of this rule to the desired `Print` interface.
        /// @param sink The output to send the information to (usually Serial).
        void print_to(Print& sink);
        
        /// @brief Get the name of this rule.
        /// @return The name.
        const char* get_name() { return rulename.c_str(); }
       
        /// @brief evaluate the expression and return the result.
        /// @return The result of the expression, or NaN if an error occured.
        te_type evaluate();

        bool compile();
    protected:
        /// @brief Update the parser with a version that likely contains references to more variables.
        /// @param base_parser A `te_parser` instance from the `RuleEngine` that is already furnaced with the available variables.
        void set_parser(te_parser base_parser){ parser = base_parser; }

        /// @brief Name of this rule 
        std::string rulename;

        /// @brief Expression formatted according to the reference (https://github.com/Blake-Madden/tinyexpr-plusplus)
        std::string expression;

        /// @brief The `te_parser` instance that will compile and evaluate the expression.
        te_parser parser;

        /// @brief Indicates if the expression is compiled correctly. 
        bool compiled;

        /// @brief Indicates if the rule should be evaluated.
        bool enabled;

        /// @brief Time in seconds between evaluations.
        uint32_t eval_interval;

        /// @brief Timestamp of the last evaluation.
        uint32_t last_evaluation;

    friend class RuleEngine;
};

Rule::Rule(
    const char *name, 
    const char *expression,
    uint32_t eval_interval,
    bool enabled, 
    te_parser baseparser,
    uint32_t last_eval
):
    rulename(name),
    expression(expression),
    eval_interval(eval_interval),
    enabled(enabled),
    parser(baseparser),
    last_evaluation(last_eval)
{
    compile();
}

void Rule::print_to(Print& sink)
{
    sink.print("name:        "); sink.println(rulename.c_str());
    sink.print("expression:  "); sink.println(expression.c_str());
    sink.print("compiled:    "); sink.println(compiled);
    sink.print("interval:    "); sink.println(eval_interval);
    sink.print("enabled:     "); sink.println(enabled);
}

te_type Rule::evaluate()
{
    
    if(compiled)
    {
        return parser.evaluate();
    }
    return te_type(NAN);
}

bool Rule::compile()
{
    try
    {
        // compile expression.
        compiled = parser.compile(expression);
    }
    catch(std::runtime_error err)
    {
        compiled = false;
        enabled = false;
    }
    if(compiled)
    {
        // remove variables that are not used in this expression from this parser instance.
        parser.remove_unused_variables_and_functions();
    }
    return compiled;
}

#endif