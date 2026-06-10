#ifndef RULEENGINE_HPP
#define RULEENGINE_HPP
#include <string>
#include <functional>
#include <initializer_list>
#include <vector>
#include <Print.h>
#include "../src/TinyExpr/tinyexpr.h"
#include "ArduinoJson.h"
#include "property.hpp"
#include "propertystore.hpp"
#include "../Debug/Debug.hpp"
#include "Rule.hpp"

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

        /// @brief Set a list of independent rules. These rules are not available to other rules in the rule engine.
        /// @param rules_list 
        void set_independent_rules(std::initializer_list<Rule*> rules_list) { independent_rules = rules_list; }

        /// @brief Start the engine
        void begin();

        /// @brief Processes a JsonDocument with one or more rules to add or update.
        /// @param obj 
        /// @details The object should have for each rule as key the name of the rule, and as 
        /// value a nested Json object containing the rrequired parameters of the rule. Exactly what parameters 
        /// depends on the specific implementation of the rule engine derived from this base class.
        void process_attributes(JsonObject obj);

        /// @brief Runs the engine. Should be implemented in derived classes.
        virtual void loop();
        
        /// @brief Print information about the rule engine to the `debug`er passed to the constructor.
        virtual void print();

        virtual BaseStore& get_rules() = 0;

        /// @brief Process a command.
        /// @param cmd 
        virtual void process_command(String& cmd);
        
    protected:
        /// @brief 
        /// @param debugger The debug interface to print messages to.
        RuleEngine(Debug* debugger);

        /// @brief Add or update a rule with the parameters contained in `pair`.
        /// @param pair The parameters needed by the rule.
        /// @return true, if the rule was constructed successfully.
        virtual bool process_rule(JsonPair pair);
        
        /// @brief All the variables have been set, so the rules can now be compiled.
        virtual void compile_rules();

        /// @brief Find rule by name
        /// @param name The name
        /// @return The rule if found, otherwise a `nullptr`.
        virtual Rule* find_rule(const char* name);

        /// @brief A base expression parser that is provided with the variables, and is copied to all 
        /// rules to be compiled with an expression.
        te_parser base_parser;

        /// @brief Stores the `te_Property` function pointer objects.
        std::vector<te_Property*> functionpointers;
        
        /// @brief A pointer to a `Debug` object that prints debug messages to the relevant outputs.
        Debug* debug;

        /// @brief The set of rules to evaluate.
        std::vector<Rule*> independent_rules;
};

RuleEngine::RuleEngine(Debug* debugger):
    debug(debugger)
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
    debug->printv("[RuleEngine]: Adding ", prop->get_name(), " to variables.");
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
        debug->printv("[RuleEngine] EXCEPTION: ", err.what());
        return;
    }
    set_variables(props...);
}

void RuleEngine::set_variables()
{
    return;
}

void RuleEngine::begin()
{
    compile_rules();
}

void RuleEngine::process_attributes(JsonObject obj)
{
    for(JsonPair pair : obj)
    {
        process_rule(pair);
    }
}

void RuleEngine::loop()
{
    for(Rule* rule : independent_rules)
    {
        if(rule->enabled)
        {
            // Its time to evaluate one
            if(millis() - rule->last_evaluation >= (rule->eval_interval * 1000ul))
            {
                rule->last_evaluation = millis();
                debug->printv("[RuleEngine] evaluating ", rule->get_name());
                rule->update();
            }
        }
    }
}

bool RuleEngine::process_rule(JsonPair pair)
{
    // JsonPair pair needs to have the following structure:
    // "rule_id": {
    //     "expression": "gerrit + henk == chaos",
    //     "eval_interval": 600,
    //     "enabled": true
    //     "address": 25
    // }
    
    const char* rule_name = pair.key().c_str();

    if(!pair.value().is<JsonObject>())
    {
        // Value is not a JsonObject.
        debug->printv("[RuleEngine] ERROR: Value is not a JsonObject.");
        return false;
    }
    
    JsonObject params = pair.value();
    for(Rule* rule : independent_rules)
    {
        // Found rule
        if(rule->rulename == rule_name)
        {
            debug->printv("[RuleEngine] Found existing rule with name ", rule_name);
            rule->modify(params);
            rule->set_parser(base_parser);
            rule->compile();

            if(!rule->compiled)
            {
                // Expression compilation error.
                debug->printv("[RuleEngine] ERROR: Expression compilation failed");
                debug->printv(rule->parser.get_last_error_position());
                debug->printv(rule->parser.get_last_error_message().c_str());
                return false;
            }
            else
            {
                debug->printv("[RuleEngine] Rule updated");
                return true;
            }
        }
    }
    debug->printv("[RuleEngine] Rule not found");
    return false;
}

void RuleEngine::compile_rules()
{
    for(Rule* rule : independent_rules)
    {
        rule->set_parser(base_parser);
        rule->compile();
    }
}

Rule *RuleEngine::find_rule(const char *name)
{
    for(Rule* rule : independent_rules)
    {
        // Rule already exists.
        if(strcmp(rule->get_name(), name) == 0)
        {
            return rule;
        }
    }
    return nullptr;
}

void RuleEngine::print()
{
    debug->printv("[RuleEngine] with ", independent_rules.size(), " rules.");

    for(Rule* rule : independent_rules)
    {
        rule->print_to(*debug);
    }
}

void RuleEngine::process_command(String &cmd)
{
    int var_sep = cmd.indexOf(':');
    if(var_sep > 0)
    {
        String name = cmd.substring(0, var_sep);
        String val  = cmd.substring(var_sep + 1);
        if(name == "eval")
        {
            // Force evaluation of rule identified by val
            Rule* rule = find_rule(val.c_str());
            if(rule != nullptr)
            {
                debug->printv("[RuleEngine] evaluating ", rule->get_name());
                rule->update();
            }
            return;
        }
        // Modify a rule
        else if(name == "set_rule")
        {
            JsonDocument doc;
            DeserializationError err = deserializeJson(doc, val);
            if(err)
            {
                debug->printv("[RuleEngine] ERROR parsing JSON");
                debug->printv(err.c_str());
            }
            else
            {
                debug->printv("[RuleEngine] processing rule.");
                process_attributes(doc.as<JsonObject>());
            }
        }
    }
}

#endif