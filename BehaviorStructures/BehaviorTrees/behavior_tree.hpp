#pragma once
#include <memory>
#include "behaviors.hpp"
#include "behavior_tree_builder.hpp"
#include "../Serialization/iserializable.hpp"

namespace fluczakAI
{
/**
 * \brief A class for a behavior tree. It can be executed using an Execute() function and a
 * behavior tree execution context.
 */
class BehaviorTree : public ISerializable
    {
    public:
        BehaviorTree(std::unique_ptr<Behavior>& rootToSet);
        /**
         * \brief This function executes its children based on their behaviors.
         * \param context - A behavior tree execution context
         */
        void Execute(BehaviorTreeContext& context) const;
        /**
         * \brief A getter for the root 
         * \return - a pointer to the root behavior
         */
        std::unique_ptr<Behavior>& GetRoot()  { return m_root; }

#if defined(NLOHMANN_JSON_VERSION_MAJOR)
        static void SerializeBehavior(nlohmann::json& json, const std::unique_ptr<Behavior>& behavior);
        nlohmann::json Serialize() override;

        void DeserializeBehavior(const nlohmann::json& json, BehaviorTreeBuilder& builder);
        void DeserializeAction(std::string& actionName, const nlohmann::json& variables, BehaviorTreeBuilder& builder) const;
        void Deserialize(nlohmann::json& json) override;
#endif
    private:
        std::unique_ptr<Behavior> m_root = {};


    };
}

