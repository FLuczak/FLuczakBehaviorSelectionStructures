#include "behavior_tree.hpp"
#include "behavior_tree_builder.hpp"

void fluczakAI::BehaviorTree::Execute(fluczakAI::BehaviorTreeContext& context) const
{
    m_root->Execute(context);
}

std::stringstream fluczakAI::BehaviorTree::Serialize() const
{
    //TODO::FINISH THIS SERIALIZATION METHOD
    return std::stringstream("");
}

std::unique_ptr<fluczakAI::BehaviorTree> fluczakAI::BehaviorTree::Deserialize(std::stringstream& stringstream)
{
    //TODO: FINISH THIS DESERIALIZATION METHOD
    BehaviorTreeBuilder builder{};
    auto temp = builder.End();
    return temp;
}

fluczakAI::BehaviorTree::BehaviorTree(std::unique_ptr<Behavior>& rootToSet)
{
    m_root.swap(rootToSet);
}
