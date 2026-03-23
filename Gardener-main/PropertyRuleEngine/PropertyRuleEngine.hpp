#ifndef PROPERTYRULEENGINE_HPP
#define PROPERTYRULEENGINE_HPP
#include <functional>
#include "property.hpp"
#include "../RuleEngine/RuleEngineBase.hpp"

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
    private:
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
        BasePropertyRule* find_rule(const char* name);
        
        /// @brief A set of rules that are created internally. This vector 'owns` the rules stored inside.
        std::vector<BasePropertyRule*> internal_rules;
        /// @brief A set of rules that are defined outside this class, This vector only stores pointers to these rules.
        std::vector<BasePropertyRule*> external_rules;
        
};

PropertyRuleEngine::PropertyRuleEngine(Debug& debugger):
    RuleEngine(&debugger)
{}

PropertyRuleEngine::~PropertyRuleEngine()
{
    for(BasePropertyRule* rule : internal_rules)
    {
        delete rule;
    }
}

void PropertyRuleEngine::loop()
{
    // Loop through all the rules 
    for(BasePropertyRule* rule : internal_rules)
    {
        if(rule->enabled)
        {
            // Its time to evaluate one
            if(millis() - rule->last_evaluation >= (rule->eval_interval * 1000ul))
            {
                rule->last_evaluation = millis();
                debug->print("[PropertyRuleEngine] evaluating ", rule->get_name());
                rule->update();   
            }
        }
    }
    for(BasePropertyRule* rule : external_rules)
    {
        
        if(rule->enabled)
        {
            // Its time to evaluate one
            if(millis() - rule->last_evaluation >= (rule->eval_interval * 1000ul))
            {
                rule->last_evaluation = millis();
                debug->print("[PropertyRuleEngine] evaluating ", rule->get_name());
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
        debug->print("[PropertyRuleEngine] ERROR: Value is not a JsonObject.");
        return false;
    }
    
    JsonObject params = pair.value();
    if(!(
        params["expression"].is<const char*>() &&
        params["eval_interval"].is<uint32_t>() &&
        params["enabled"].is<bool>()
    )){
        // JsonObject does not contain correct keys.
        debug->print("[PropertyRuleEngine] ERROR: JsonObject does not contain correct keys.");
        return false;
    }  
    
    uint32_t last_evaluation = 0;

    BasePropertyRule* existing_rule = find_rule(rule_name);
    
    if(existing_rule == nullptr)
    {
        // No rule whatsoever. Create new one.
        PropertyRule<te_type>* newrule = new PropertyRule<te_type>(
            rule_name,
            params["expression"].as<const char*>(),
            params["eval_interval"].as<uint32_t>(),
            params["enabled"].as<bool>(),
            base_parser,
            0
        );
        
        if(!newrule->compiled)
        {
            // Expression compilation error.
            debug->print("[PropertyRuleEngine] ERROR: Expression compilation failed");
            debug->print(newrule->parser.get_last_error_position());
            debug->print(newrule->parser.get_last_error_message().c_str());
            delete newrule;
            return false;
        }
        debug->print("[PropertyRuleEngine] Adding new rule");
        internal_rules.push_back(newrule);
        set_variables(newrule);
        compile_rules();
        return true;
    }
    else
    {
        debug->print("[PropertyRuleEngine] Modifying existing rule");
        // Modify parameters in place.
        existing_rule->expression = params["expression"].as<std::string>();
        existing_rule->eval_interval = params["eval_interval"].as<uint32_t>();
        existing_rule->enabled = params["enabled"].as<bool>();
        existing_rule->compiled = false;
        compile_rules();
        return true;
    }

}

void PropertyRuleEngine::print()
{
    debug->print(
        "[PropertyRuleEngine] with ", 
        internal_rules.size(), 
        " internal rules and ", 
        external_rules.size(), 
        " external rules."
    );

    for(BasePropertyRule* rule : internal_rules)
    {
        rule->print_to(*debug);
        debug->print();
    }
    for(BasePropertyRule* rule : external_rules)
    {
        rule->print_to(*debug);
        debug->print();
    }
}

template <typename T, typename... Args>
void PropertyRuleEngine::set_ext_rules(PropertyRule<T>* rule, Args... rules)
{
    debug->print("[PropertyRuleEngine]: Adding ", rule->get_name(), " to rules.");
    external_rules.push_back(rule);
    // Pass the rule to the parser variables so the result of the rule's expression is available to other rules.
    set_variables(rule);

    set_ext_rules(rules...);
}

void PropertyRuleEngine::set_ext_rules()
{
    // All external rules have been added. Provide all these rules with the updated parser and compile them.
    for(BasePropertyRule* rule : external_rules)
    {
        rule->set_parser(base_parser);
        rule->compile();
    }
    return;
}

void PropertyRuleEngine::compile_rules()
{
    for(BasePropertyRule* rule : external_rules)
    {
        if(!rule->compiled)
        {
            rule->set_parser(base_parser);
            rule->compile();
        }
    }
    for(BasePropertyRule* rule : internal_rules)
    {
        if(!rule->compiled)
        {
            rule->set_parser(base_parser);
            rule->compile();
        }
    }
}

BasePropertyRule* PropertyRuleEngine::find_rule(const char *name)
{
    for(BasePropertyRule* rule : external_rules)
    {
        // Rule already exists.
        if(strcmp(rule->get_name(), name) == 0)
        {
            return rule;
        }
    }
    for(BasePropertyRule* rule : internal_rules)
    {
        // Rule already exists.
        if(strcmp(rule->get_name(), name) == 0)
        {
            return rule;
        }
    }return nullptr;
}

#endif