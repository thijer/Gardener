#ifndef RULEENGINEBASE_HPP
#define RULEENGINEBASE_HPP
#include <string>
#include <functional>
#include <Print.h>
#include "../src/TinyExpr/tinyexpr.h"
#include "ArduinoJson.h"
#include "property.hpp"
#include "debug.hpp"

class Rule
{
    public:
        Rule(
            const char* name, 
            const char* expression, 
            uint32_t eval_interval,
            bool enabled,
            te_parser baseparser,
            uint32_t last_eval = 0
        );
        
        void print_to(Print& sink);
        const char* get_name() { return name.c_str(); }
        te_type evaluate();
    protected:
        std::string name;
        std::string expression;
        te_parser parser;
        bool compiled;
        bool enabled;
        uint32_t eval_interval;
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
        compiled = parser.compile(expression);
    }
    catch(std::runtime_error err)
    {
        // PRINT("[Rule] EXCEPTION: ", err.what());
        compiled = false;
        enabled = false;
    }
    if(compiled)
    {
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
        template<typename T>
        explicit te_Property(Property<T>* prop, const te_variable_flags type):
            te_expr(type),
            func(std::bind(&extract_property_value<T>, prop))
        {}
        std::function<te_type()> func;
    private:
};

te_type get_property_value(const te_expr* context)
{
    const te_Property* ptr = static_cast<const te_Property*>(context);
    return ptr->func();
}


class RuleEngine 
{
    public:
        ~RuleEngine();
        template<typename T, typename... Args>
        void set_variables(Property<T>* prop, Args... props);
        void set_variables();
        
        void begin(Debug& debugger = emptydebug);
        void process_attributes(JsonObject obj);
        virtual void loop() = 0;
        virtual void print() = 0;
        
    protected:
        RuleEngine();
        virtual bool process_rule(JsonPair pair) = 0;
        
        te_parser base_parser;
        std::vector<te_Property*> functionpointers;
        Debug* debug;
};

RuleEngine::RuleEngine()
{}

RuleEngine::~RuleEngine()
{
    for(auto p : functionpointers)
    {
        delete p;
    }
}

template <typename T, typename... Args>
void RuleEngine::set_variables(Property<T>* prop, Args... props)
{
    debug->print("[RuleEngine]: Adding ", prop->get_name(), " to variables.");
    te_Property* ptr = new te_Property(prop, TE_DEFAULT);
    functionpointers.push_back(ptr);
    try
    {
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