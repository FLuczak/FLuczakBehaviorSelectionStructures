#pragma once

#include <functional>
#include <memory>
#include <string>

namespace fluczakAI
{
class FunctionWrapperBase
{
public:
    virtual ~FunctionWrapperBase(){};
};


template <class T,typename ... Args>
class CreatorFunction : public FunctionWrapperBase
{
public:
    CreatorFunction(std::function<std::unique_ptr<T>(Args...)> newFunc) : function(newFunc){};
    std::function<std::unique_ptr<T>(Args...)> function;
};

template<typename T>
class GenericFactory
{
public:
    std::vector<std::string> GetKeys() const
    {
        std::vector<std::string> toReturn{};
        for (auto& pair : creators)
        {
            toReturn.push_back(pair.first);
        }
        return toReturn;
    }

    static GenericFactory& Instance()
    {
        static GenericFactory factory;
        return factory;
    }

    template <typename newType,typename... Args>
    void RegisterProduct(const std::string& name)
    {
        creators[name] = std::make_unique<CreatorFunction<newType, Args...>>([](Args... args){ return std::make_unique<newType>(args...); });
    }

    template<typename... Args>
    std::unique_ptr<T> CreateProduct(const std::string& name,Args... args) const
    {
        auto it = creators.find(name);
        if (it != creators.end())
        {
            auto functionPtr = reinterpret_cast<CreatorFunction<T, Args...>*>(it->second.get());
            return functionPtr->function(std::forward<Args>(args)...);
        }
        return nullptr;  // or throw an exception
    }

private:
    std::unordered_map<std::string, std::unique_ptr<FunctionWrapperBase>> creators;

    GenericFactory() = default;
};
}

// Macros for registration
#define REGISTER_STATE(DerivedType)                                   \
    static const bool DerivedType##Registered = []                                \
    {                                                                             \
        fluczakAI::GenericFactory<AI::State>::Instance().RegisterProduct<DerivedType>(#DerivedType); \
        return true;                                                              \
    }()


// Macros for registration
#define REGISTER_ACTION(DerivedType)                                   \
    static const bool DerivedType##Registered = []                                \
    {                                                                             \
        fluczakAI::GenericFactory<AI::BehaviorTreeAction>::Instance().RegisterProduct<DerivedType>(#DerivedType); \
        return true;                                                              \
    }()




