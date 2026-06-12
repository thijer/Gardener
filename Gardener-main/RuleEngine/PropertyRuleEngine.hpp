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
        PropertyRuleEngine(std::initializer_list<Rule*> rules_list, Debug& debugger = emptydebug);
        ~PropertyRuleEngine();
        
        /// @brief Add a set of external `PropertyRule`s to the rule engine.
        /// @tparam T The type of the `PropertyRule`
        /// @tparam ...Args The types  of the remaining `PropertyRule`s that are processed in a recursive call of this function. 
        /// @param rule The `PropertyRule`. 
        /// @param ...rules The remaining `PropertyRule`s.
        template<typename T, typename... Args>
        void set_ext_rules(PropertyRule<T>* rule, Args... more_rules);
        
        /// @brief End of the recursive `set_variables` function.
        void set_ext_rules();
};

PropertyRuleEngine::PropertyRuleEngine(std::initializer_list<Rule*> rules_list, Debug& debugger):
    RuleEngine(rules_list, debugger)
{}

PropertyRuleEngine::~PropertyRuleEngine()
{}

template <typename T, typename... Args>
void PropertyRuleEngine::set_ext_rules(PropertyRule<T>* rule, Args... more_rules)
{
    debug->printv("[PropertyRuleEngine]: Adding ", rule->get_name(), " to rules.");
    rules.push_back(rule);
    
    // Pass the rule to the parser variables so the result of the rule's expression is available to other rules.
    set_variables(rule->get_property());

    set_ext_rules(more_rules...);
}

void PropertyRuleEngine::set_ext_rules()
{
    // All external rules have been added. Provide all these rules with the updated parser and compile them.
    for(Rule* rule : rules)
    {
        rule->set_parser(base_parser);
        rule->compile();
    }
    
    rules_basestore.set(rules);
    return;
}

#endif