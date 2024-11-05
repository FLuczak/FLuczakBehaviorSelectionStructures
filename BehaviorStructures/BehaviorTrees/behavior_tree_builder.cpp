
#include "behavior_tree_builder.hpp"
#include <memory>
#include "behaviors.hpp"


fluczakAI::BehaviorTreeBuilder& fluczakAI::BehaviorTreeBuilder::Selector()
{
    auto temp = std::make_unique<fluczakAI::Selector>(id++);
    AddBehavior(std::move(temp));
    return *this;
}

void fluczakAI::BehaviorTreeBuilder::AddBehavior(std::unique_ptr<fluczakAI::Behavior> behavior)
{
    const auto ptr = behavior.get();
    if (m_treeRoot == nullptr)
    {
        m_treeRoot = std::move(behavior);
    }
    else
    {
        m_nodeStack.top()->AddChild(std::move(behavior));
    }

    m_nodeStack.push(ptr);
}

fluczakAI::BehaviorTreeBuilder& fluczakAI::BehaviorTreeBuilder::Sequence()
{
    auto temp = std::make_unique<fluczakAI::Sequence>(id++);
    AddBehavior(std::move(temp));
    return *this;
}

fluczakAI::BehaviorTreeBuilder& fluczakAI::BehaviorTreeBuilder::Repeater(int numRepeats)
{
    auto temp = std::make_unique<fluczakAI::Repeater>(id++, numRepeats);
    AddBehavior(std::move(temp));
    return *this;
}

fluczakAI::BehaviorTreeBuilder& fluczakAI::BehaviorTreeBuilder::Inverter()
{
    auto temp = std::make_unique<fluczakAI::Inverter>(id++);
    AddBehavior(std::move(temp));
    return *this;
}

fluczakAI::BehaviorTreeBuilder& fluczakAI::BehaviorTreeBuilder::AlwaysSucceed()
{
    auto temp = std::make_unique<fluczakAI::AlwaysSucceed>(id++);
    AddBehavior(std::move(temp));
    return *this;
}

fluczakAI::BehaviorTreeBuilder& fluczakAI::BehaviorTreeBuilder::UntilFail()
{
    auto temp = std::make_unique<fluczakAI::UntilFail>(id++);
    AddBehavior(std::move(temp));
    return *this;
}

fluczakAI::BehaviorTreeBuilder& fluczakAI::BehaviorTreeBuilder::Back()
{
    m_nodeStack.pop();
    return *this;
}

std::unique_ptr<fluczakAI::BehaviorTree> fluczakAI::BehaviorTreeBuilder::End()
{
    auto behavior_tree = std::make_unique<fluczakAI::BehaviorTree>(m_treeRoot);
    return std::move(behavior_tree);
}

#if defined(NLOHMANN_JSON_VERSION_MAJOR)
nlohmann::json fluczakAI::BehaviorTreeBuilder::Serialize()
{
}

void fluczakAI::BehaviorTreeBuilder::Deserialize(nlohmann::json& json)
{
}
#endif
