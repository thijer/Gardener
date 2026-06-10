#ifndef PROPERTYRULEENGINE_HPP
#define PROPERTYRULEENGINE_HPP
#include <functional>
#include "ArduinoJson.h"
#include "property.hpp"
#include "propertystore.hpp"
#include "Rule.hpp"
#include "PropertyRule.hpp"
#include "RuleEngine.hpp"
#include "../VectorBaseStore/VectorBaseStore.hpp"
#include "../Debug/Debug.hpp"

/// @brief A `RuleEngine` implementation that manages internal and external `PropertyRule`s.
class PropertyRuleEngine: public RuleEngine
{
    public:
        PropertyRuleEngine(Debug& debugger = emptydebug);
        ~PropertyRuleEngine();
        void loop();
        void print();

        /// @brief Add a set of external `PropertyRule`s to the rule engine.
        /// @tparam T The type of the `PropertyRule`
        /// @tparam ...Args The types  of the remaining `PropertyRule`s that are processed in a recursive call of this function. 
        /// @param rule The `PropertyRule`. 
        /// @param ...rules The remaining `PropertyRule`s.
        template<typename T, typename... Args>
        void set_ext_rules(PropertyRule<T>* rule, Args... rules);
        
        /// @brief End of the recursive `set_variables` function.
        void set_ext_rules();

        BaseStore& get_rules();
    protected:
        /// @brief Add or update a rule with the parameters contained in `pair`.
        /// @param pair The parameters needed by the rule.
        /// @return true, if the rule was constructed successfully.
        /// 
        /// The key-value pair `pair` needs to have the following entries:
        /// "rule_id": {
        ///     "expression": <string>,
        ///     "eval_interval": <int>,
        ///     "enabled": <bool>
        /// }
        bool process_rule(JsonPair pair);
    private:
        /// @brief Recompile all uncompiled rules with an updated parser.
        void compile_rules();

        /// @brief Find rule by name
        /// @param name The name
        /// @return The rule if found, otherwise a `nullptr`.
        Rule* find_rule(const char* name);
        
        /// @brief A set of rules that are defined outside this class. This vector only stores pointers to these rules.
        std::vector<Rule*> external_rules;
        // Store the `Property`component of external rules so we can request them from thingsboard.
        std::vector<BaseProperty*> external_properties;
        
        VectorBaseStore rules_basestore;
        
};

PropertyRuleEngine::PropertyRuleEngine(Debug& debugger):
    RuleEngine(&debugger)
{}

PropertyRuleEngine::~PropertyRuleEngine()
{}

void PropertyRuleEngine::loop()
{
    RuleEngine::loop();
    
    // Loop through all the rules 
    for(Rule* rule : external_rules)
    {
        
        if(rule->enabled)
        {
            // Its time to evaluate one
            if(millis() - rule->last_evaluation >= (rule->eval_interval * 1000ul))
            {
                rule->last_evaluation = millis();
                debug->printv("[PropertyRuleEngine] evaluating ", rule->get_name());
                rule->update();
            }
        }
    }
}

bool PropertyRuleEngine::process_rule(JsonPair pair)
{
    // JsonPair pair needs to have the following structure:
    // "rule_id": {
    //     "expression": <string>,
    //     "eval_interval": <int>,
    //     "enabled": <bool>
    // }
    
    const char* rule_name = pair.key().c_str();

    if(!pair.value().is<JsonObject>())
    {
        // Value is not a JsonObject.
        debug->printv("[PropertyRuleEngine] ERROR: Value is not a JsonObject.");
        return false;
    }
    Rule* existing_rule = find_rule(rule_name);
    JsonObject params = pair.value();
    
    if(existing_rule == nullptr)
    {
        return false;
    }
    else
    {
        debug->printv("[PropertyRuleEngine] Modifying existing rule");
        bool success = existing_rule->modify(params);
        if(!success)
        {
            debug->printv("[PropertyRuleEngine] ERROR: JsonObject does not contain correct keys.");
            return success;
        }
        compile_rules();
        return success;
    }

}

void PropertyRuleEngine::print()
{
    debug->printv(
        "[PropertyRuleEngine] with ", 
        independent_rules.size(), 
        " independent rules and ", 
        external_rules.size(), 
        " external rules."
    );
    for(Rule* rule : independent_rules)
    {
        rule->print_to(*debug);
        debug->println();
    }

    for(Rule* rule : external_rules)
    {
        rule->print_to(*debug);
        debug->println();
    }
}

template <typename T, typename... Args>
void PropertyRuleEngine::set_ext_rules(PropertyRule<T>* rule, Args... rules)
{
    debug->printv("[PropertyRuleEngine]: Adding ", rule->get_name(), " to rules.");
    external_rules.push_back(rule);
    external_properties.push_back(rule);
    // Pass the rule to the parser variables so the result of the rule's expression is available to other rules.
    set_variables(rule);

    set_ext_rules(rules...);
}

void PropertyRuleEngine::set_ext_rules()
{
    // All external rules have been added. Provide all these rules with the updated parser and compile them.
    for(Rule* rule : external_rules)
    {
        rule->set_parser(base_parser);
        rule->compile();
    }
    
    rules_basestore.set(external_properties);
    return;
}

BaseStore& PropertyRuleEngine::get_rules()
{
    return rules_basestore;
}

void PropertyRuleEngine::compile_rules()
{
    RuleEngine::compile_rules();
    for(Rule* rule : external_rules)
    {
        if(!rule->compiled)
        {
            rule->set_parser(base_parser);
            rule->compile();
        }
    }
}

Rule* PropertyRuleEngine::find_rule(const char *name)
{
    for(Rule* rule : independent_rules)
    {
        // Rule already exists.
        if(strcmp(rule->get_name(), name) == 0)
        {
            return rule;
        }
    }
    for(Rule* rule : external_rules)
    {
        // Rule already exists.
        if(strcmp(rule->get_name(), name) == 0)
        {
            return rule;
        }
    }
    return nullptr;
}

#endif