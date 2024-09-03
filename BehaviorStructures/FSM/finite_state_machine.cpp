#include "finite_state_machine.hpp"

#include <sstream>
#include "../Blackboards/comparator.hpp"

bool fluczakAI::TransitionData::CanTransition(const fluczakAI::StateMachineContext& context)const
{
   for (const auto& comparator : comparators)
    {
        if (!comparator->Evaluate(*context.blackboard))
        {
            return false;
        }
    }
    return true;
}

void fluczakAI::FiniteStateMachine::Execute(StateMachineContext& context)
{
    if (m_states.empty()) return;

    if (!context.currentState.has_value())
    {
        context.currentState = m_defaultState.value();
        m_states[context.currentState.value()]->Initialize(context);
    }

    const auto currentStateIndex = context.currentState.value();

    for (const auto& transitionData : m_transitions[currentStateIndex])
    {
        if (!transitionData.CanTransition(context))
        {
            continue;
        }

        auto typeIndex = transitionData.StateToGoTo();
        const std::unique_ptr<State>& endedState = m_states.at(currentStateIndex);
        endedState->End(context);

        context.currentState = typeIndex;

        if (!context.currentState.has_value()) continue;
        if (context.currentState.value() >= m_states.size()) continue;

        const std::unique_ptr<State>& stateToInit = m_states.at(context.currentState.value());
        stateToInit->Initialize(context);
    }

    m_states[currentStateIndex]->Update(context);
}

void fluczakAI::FiniteStateMachine::SetCurrentState(size_t stateToSet, StateMachineContext& context) const
{
    if (context.currentState.has_value())
    {
        const auto currentStateIndex = context.currentState.value();
        const std::unique_ptr<State>& endedState = m_states.at(currentStateIndex);
        endedState->End(context);
    }

    context.currentState = stateToSet;
    const std::unique_ptr<State>& stateToInit = m_states.at(context.currentState.value());
    stateToInit->Initialize(context);
}

std::stringstream fluczakAI::FiniteStateMachine::Serialize() const
{
    //TODO: FILL THIS IN
    return std::stringstream();
}


std::unique_ptr<fluczakAI::FiniteStateMachine> fluczakAI::FiniteStateMachine::DeserializeFromStringStream(std::stringstream& stream)
{
    //TODO: FILL THIS IN WITH DESERIALIZATION IMPLEMENTATION 
    std::unique_ptr<FiniteStateMachine> temp = std::make_unique<FiniteStateMachine>();
    return std::move(temp);
}

