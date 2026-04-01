#ifndef VECTORBASESTORE_HPP
#define VECTORBASESTORE_HPP
#include <vector>
#include "property.hpp"
#include "propertystore.hpp"

/// @brief A helper class to pass `std::vector`s as `BaseStore`s. 
class VectorBaseStore: public BaseStore
{
    public:
        /// @brief 
        /// @tparam T The type of the vector. This should be castable to BaseProperty* 
        /// @param props A vector with pointers to a `BaseProperty` derived class.
        template<typename T>
        VectorBaseStore(std::vector<T>& props):
            BaseStore(props.size()),
            properties(props.begin(), props.end())
        {}

        VectorBaseStore():
            BaseStore(0)
        {}

        /// @brief 
        /// @tparam T The type of the vector. This should be castable to BaseProperty* 
        /// @param props A vector with pointers to a `BaseProperty` derived class.
        template<typename T>
        void set(std::vector<T>& props) { properties = std::vector<BaseProperty*>(props.begin(), props.end()); }
        
        // Convert vector iterators to BaseStore iterators.
        Iterator begin() { return Iterator(static_cast<BaseProperty** const>(properties.begin().base())); }
        Iterator end()   { return Iterator(static_cast<BaseProperty** const>(properties.end().base())); }
        
        BaseProperty* get_property(uint32_t i)
        {
            if(i >= properties.size()) return nullptr;
            else return static_cast<BaseProperty*>(properties[i]);
        }
    private:
        std::vector<BaseProperty*> properties;
};


#endif