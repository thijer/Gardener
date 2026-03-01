#ifndef RULEENGINEBASE_HPP
#define RULEENGINEBASE_HPP
#include <string>
#include <functional>
#include <Print.h>
#include "../src/TinyExpr/tinyexpr.h"
#include "ArduinoJson.h"
#include "property.hpp"
#include "debug.hpp"

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
        const char* get_name() { return name.c_str(); }
       
        /// @brief evaluate the expression and return the result.
        /// @return The result of the expression, or NaN if an error occured.
        te_type evaluate();
    protected:
        /// @brief Name of this rule 
        std::string name;

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
    name(name),
    expression(expression),
    eval_interval(eval_interval),
    enabled(enabled),
    parser(baseparser),
    last_evaluation(last_eval)
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
}

void Rule::print_to(Print& sink)
{
    sink.print("name:        "); sink.println(name.c_str());
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

/// @brief Helper function to extract a te_type type value from a templated `Property` instance.
/// @tparam T The type of the `Property`, usually `bool`, `int32_t`, or `float`.
/// @param prop The `Property` to get the value from.
/// @return The value stored in the `Property`
template<typename T>
te_type extract_property_value(Property<T>* prop)
{
    return te_type(prop->get());
}

/// @brief This class stores a function pointer to the `extract_property_value` function that is bound to a specific `Property`.
/// This can then be passed by TinyExpr to the `get_property_value` function as context to get the property value.
class te_Property: public te_expr
{
    public:
        /// @brief Construct the object.
        /// @tparam T The type of the `Property`
        /// @param prop The `Property` to add to the parser variables.
        template<typename T>
        explicit te_Property(Property<T>* prop):
            te_expr(TE_DEFAULT),
            func(std::bind(&extract_property_value<T>, prop))
        {}
        
        /// @brief Pointer to the `extract_property_value` function bound to a specific `Property`.
        std::function<te_type()> func;
    private:
};

/// @brief This function is called by `te_parser` when the parser needs the value of the `Property` 
/// associated with the `te_Property` instance that is passed here as context.
/// @param context a `te_Property` instance that contains the function bound to the desired `Property`.
/// @return The value of the `Property`.
te_type get_property_value(const te_expr* context)
{
    const te_Property* ptr = static_cast<const te_Property*>(context);
    return ptr->func();
}


/// @brief A base class for a rule engine that initializes a parser and manages a collection of rules.
class RuleEngine 
{
    public:
        ~RuleEngine();

        /// @brief Add a set of `Property`s to the expression parser.
        /// @tparam T The type of the `Property`
        /// @tparam ...Args The types  of the remaining `Property`s that are processed in a recursive call of this function. 
        /// @param prop The `Property`. 
        /// @param ...props The remaining `Property`s.
        template<typename T, typename... Args>
        void set_variables(Property<T>* prop, Args... props);
        
        /// @brief End of the recursive `set_variables` function.
        void set_variables();
        
        /// @brief Start the engine
        /// @param debugger A `Debug` instance to send the debug messages to.
        void begin(Debug& debugger = emptydebug);

        /// @brief Processes a JsonDocument with one or more rules to add or update.
        /// @param obj 
        /// @details The object should have for each rule as key the name of the rule, and as 
        /// value a nested Json object containing the rrequired parameters of the rule. Exactly what parameters 
        /// depends on the specific implementation of the rule engine derived from this base class.
        void process_attributes(JsonObject obj);
        /// @brief Runs the engine. Should be implemented in derived classes.
        virtual void loop() = 0;
        
        /// @brief Print information about the rule engine to the `debug`er passed to `begin()`.
        virtual void print() = 0;
        
    protected:
        /// @brief 
        RuleEngine();
        /// @brief Add or update a rule with the parameters contained in `pair`.
        /// @param pair The parameters needed by the rule.
        /// @return true, if the rule was constructed successfully.
        virtual bool process_rule(JsonPair pair) = 0;
        
        /// @brief A base expression parser that is provided with the variables, and is copied to all 
        /// rules to be compiled with an expression.
        te_parser base_parser;
        /// @brief Stores the `te_Property` function pointer objects.
        std::vector<te_Property*> functionpointers;
        /// @brief A pointer to a `Debug` object that prints debug messages to the relevant outputs.
        Debug* debug;
};

RuleEngine::RuleEngine()
{}

RuleEngine::~RuleEngine()
{
    for(auto p : functionpointers)
    {
        // Delete functionpointers.
        delete p;
    }
}

template <typename T, typename... Args>
void RuleEngine::set_variables(Property<T>* prop, Args... props)
{
    debug->print("[RuleEngine]: Adding ", prop->get_name(), " to variables.");
    // Construct a function pointer object for this `Property`.
    te_Property* ptr = new te_Property(prop);
    functionpointers.push_back(ptr);
    try
    {
        // Add the functionpointer to the parser.
        base_parser.add_variable_or_function({prop->get_name(), get_property_value, TE_DEFAULT, ptr});
    }
    catch (std::exception err)
    {
        debug->print("[RuleEngine] EXCEPTION: ", err.what());
        return;
    }
    set_variables(props...);
}

void RuleEngine::set_variables()
{
    return;
}

void RuleEngine::begin(Debug& debugger)
{
    debug = &debugger;
}

void RuleEngine::process_attributes(JsonObject obj)
{
    for(JsonPair pair : obj)
    {
        process_rule(pair);
    }
}


#endif