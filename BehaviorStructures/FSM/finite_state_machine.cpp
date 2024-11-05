#include "finite_state_machine.hpp"

#include <sstream>
#include "../Blackboards/comparator.hpp"
#include "../Serialization/editor_variables.hpp"
#include "../Serialization/generic_factory.hpp"

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

#if defined(NLOHMANN_JSON_VERSION_MAJOR)
void fluczakAI::FiniteStateMachine::DeserializeStates(const nlohmann::json& json)
{
    for (const auto& state : json)
    {
        std::string name = state["name"].get<std::string>();
        name.erase(std::remove_if(name.begin(), name.end(), [](unsigned char c) { return std::isspace(c); }), name.end());

        bool isDefault = false;

        isDefault = state["default"].get<bool>();
        auto stateInstance = GenericFactory<State>::Instance().CreateProduct(name);

        if (stateInstance == nullptr) continue;

        if (isDefault)
        {
            m_defaultState = state;
        }

        for (auto& editorVariable : state["editor-variables"])
        {
            if (stateInstance->editorVariables.find(editorVariable["name"]) == stateInstance->editorVariables.end()) continue;
            stateInstance->editorVariables[editorVariable["name"]]->Deserialize(editorVariable["value"]);
        }

        m_states.push_back(std::move(stateInstance));
    }

    if (m_states.empty()) return;
    if (!m_defaultState.has_value())
    {
        m_defaultState = 0;
    }
}

void fluczakAI::FiniteStateMachine::DeserializeTransitions(const nlohmann::json& json,TransitionData& tempData) const
{
    for (const auto& i : json["comparators"])
    {
        std::stringstream stream = std::stringstream(i.get<std::string>());
        std::string comparisonKey;
        std::string typeName;
        int comparisonType;
        stream >> comparisonKey;
        stream >> typeName;
        stream >> comparisonType;

        if (typeName == "float")
        {
            float value;
            stream >> value;
            tempData.comparators.push_back(std::make_unique<Comparator<float>>(comparisonKey, static_cast<ComparisonType>(comparisonType), value));
        }
        if (typeName == "double")
        {
            double value;
            stream >> value;
            tempData.comparators.push_back(std::make_unique<Comparator<double>>(comparisonKey, static_cast<ComparisonType>(comparisonType), value));
        }
        if (typeName == "bool")
        {
            bool value;
            stream >> value;
            tempData.comparators.push_back(std::make_unique<Comparator<bool>>(comparisonKey, static_cast<ComparisonType>(comparisonType), value));
        }
        if (typeName == "int")
        {
            int value;
            stream >> value;
            tempData.comparators.push_back(std::make_unique<Comparator<int>>(comparisonKey, static_cast<ComparisonType>(comparisonType), value));
        }
        if (typeName.find("string") != std::string::npos)
        {
            std::string value;
            stream >> value;
            tempData.comparators.push_back(std::make_unique<Comparator<std::string>>(comparisonKey, static_cast<ComparisonType>(comparisonType), value));
        }
    }

}

void fluczakAI::FiniteStateMachine::DeserializeTransitionData(const nlohmann::json& json)
{
    for (const auto& transition : json)
    {
        size_t from = transition["from"].get<size_t>();
        for (int j = 0; j < transition["transitions"].size(); j++)
        {
            size_t stateTo = transition["transitions"][j]["to"];
            auto tempData = TransitionData(stateTo);

            DeserializeTransitions(transition["transitions"][j], tempData);

            auto transitionData = std::find_if(m_transitions[from].begin(), m_transitions[from].end(),[stateTo](const TransitionData& data) { return data.StateToGoTo() == stateTo; });
            if (transitionData == m_transitions[from].end())
            {
                m_transitions[from].push_back(std::move(tempData));
            }
            else
            {
                transitionData->comparators.push_back(std::move(tempData.comparators[0]));
            }
        }
    }
}

void fluczakAI::FiniteStateMachine::Deserialize(nlohmann::json& json)
{
	DeserializeStates(json["states"]);
    DeserializeTransitionData(json["transition-data"]);
}

void fluczakAI::FiniteStateMachine::SerializeTransitions(nlohmann::json& transitions) const
{
	for (auto& pair : m_transitions)
	{
		nlohmann::json transitionData = nlohmann::json::object();
		transitionData["from"] = pair.first;
		std::string fromIndex = std::to_string(pair.first);

		for (const auto& data : pair.second)
		{
			nlohmann::json transition = nlohmann::json::object();
			transition["to"] = data.StateToGoTo();
			nlohmann::json comparators = nlohmann::json::array();

			for (const auto& comparator : data.comparators)
			{
				comparators.emplace_back(comparator->ToString());
			}

			transition["comparators"] = comparators;
			transitionData["transitions"].push_back(transition);
		}
		transitions.push_back(transitionData);
	}
}

void fluczakAI::FiniteStateMachine::SerializeStates(nlohmann::json& states)const
{
	int index = 0;
	for (auto& state : m_states)
	{
		nlohmann::json stateObject = nlohmann::json::object();
		auto name = std::string(typeid(*state).name());

		std::size_t i = name.find("class");
		if (i != std::string::npos)
		{
			name.erase(i, 5);
		}

		i = name.find("AI::");
		if (i != std::string::npos)
		{
			name.erase(i, 4);
		}

		stateObject["default"] = m_defaultState.value() == index;

		stateObject["name"] = name;
		stateObject["editor-variables"] = nlohmann::json();

		for (auto& variable : state->editorVariables)
		{
			nlohmann::json jsonVariable;
			jsonVariable["name"] = variable.first;
			jsonVariable["value"] = variable.second->ToString();
			stateObject["editor-variables"].push_back(jsonVariable);
		}

		states.push_back(stateObject);
		index++;
	}
}

nlohmann::json fluczakAI::FiniteStateMachine::Serialize()
{
    nlohmann::json json;

    json["version"] = "1.0";
    json["behavior-structure-type"] = "FSM";

	nlohmann::json states = nlohmann::json::array();
    nlohmann::json transitions = nlohmann::json::array();

    SerializeStates(states);
    SerializeTransitions(transitions);
    json["states"] = states;
    json["transition-data"] = transitions;
    return json;
}
#endif



