#include "behavior_tree.hpp"
#include "behavior_tree_builder.hpp"
#include "../Serialization/editor_variables.hpp"
#include "behaviors.hpp"
#include "../Serialization/generic_factory.hpp"

void fluczakAI::BehaviorTree::Execute(fluczakAI::BehaviorTreeContext& context) const
{
    m_root->Execute(context);
}

#if defined(NLOHMANN_JSON_VERSION_MAJOR)

void fluczakAI::BehaviorTree::SerializeBehavior(nlohmann::json& json, const std::unique_ptr<Behavior>& behavior)
{
    nlohmann::json node;
    node["children"] = nlohmann::json::array({});

    auto name = std::string(typeid(*behavior).name());

    size_t pos = name.find('<');

    if (pos != std::string::npos)
    {
        name = name.substr(0, pos);
    }

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

    const auto action = dynamic_cast<const fluczakAI::BehaviorTreeAction*>(behavior.get());

    node["name"] = name;

    if (action != nullptr)
    {
        node["name"] = "Action";
        node["type"] = name;
        node["editor-variables"] = nlohmann::json();

        for (auto& variable : action->editorVariables)
        {
            nlohmann::json jsonVariable;
            jsonVariable["name"] = variable.first;
            jsonVariable["value"] = variable.second->ToString();
            node["editor-variables"].push_back(jsonVariable);
        }
    }

    const auto composite = dynamic_cast<const Composite*>(behavior.get());
    const auto decorator = dynamic_cast<const Decorator*>(behavior.get());
    if (composite != nullptr)
    {
        for (const auto& element : composite->GetChildren())
        {
            SerializeBehavior(node, element);
        }
    }
    else if (decorator != nullptr)
    {
        decorator->Serialize(node);
        SerializeBehavior(node, decorator->GetChild());
    }

    nlohmann::json children = json.at("children");
    children.push_back(node);
    json["children"] = children;
}

void fluczakAI::BehaviorTree::DeserializeAction(std::string& actionName,const nlohmann::json& variables,BehaviorTreeBuilder& builder) const
{
    actionName.erase(std::remove_if(actionName.begin(), actionName.end(),
        [](unsigned char c) { return std::isspace(c); }),
        actionName.end());

	auto temp = GenericFactory<fluczakAI::BehaviorTreeAction>::Instance().CreateProduct(actionName);
    if (temp != nullptr)
    {
        temp->SetId(builder.GetId()+1);

        for (auto variable : variables)
        {
            if (temp->editorVariables.find(variable["name"]) == temp->editorVariables.end()) continue;
            temp->editorVariables[variable["name"]]->Deserialize(variable["value"]);
        }

        builder.AddBehavior(std::move(temp));
        return;
    }

    builder.AddBehavior(std::make_unique<BehaviorTreeAction>());
}

nlohmann::json fluczakAI::BehaviorTree::Serialize()
{
    nlohmann::json toReturn;

    toReturn["version"] = "1.0";
    toReturn["children"] = nlohmann::json::array();
    toReturn["behavior-structure-type"] = "BehaviorTree";

    SerializeBehavior(toReturn, m_root);
    return toReturn;
}

#define PARSE(type, comparisonType) \
    type value;                                 \
    stream >> value;                            \
    Comparison(fluczakAI::Comparator<type>(comparisonKey, static_cast<ComparisonType>(comparisonType), value));

void fluczakAI::BehaviorTree::DeserializeBehavior(const nlohmann::json& json, BehaviorTreeBuilder& builder)
{
    std::string name = json.at("name").get<std::string>();
    name.erase(std::remove_if(name.begin(), name.end(), isspace), name.end());

    nlohmann::json children = json["children"];
    size_t numChildren = children.size();


    if (name == "Selector")
    {
        builder.Selector();
    }
    if (name == "Sequence")
    {
        builder.Sequence();
    }
    if (name == "Repeater")
    {
        int numRepeats = json["num-repeats"];
        builder.Repeater(numRepeats);
    }
    if (name == "Inverter")
    {
        builder.Inverter();
    }
    if (name == "AlwaysSucceed")
    {
        builder.AlwaysSucceed();
    }
    if (name == "UntilFail")
    {
        builder.UntilFail();
    }
    if (name.find("Comparison") != std::string::npos)
    {
        std::stringstream stream = std::stringstream(json["comparator"].dump());

        std::string comparisonKey;
        std::string typeName;
        int comparisonType;

        stream >> comparisonKey;
        stream >> typeName;
        stream >> comparisonType;

        auto new_end = std::remove_if(comparisonKey.begin(), comparisonKey.end(), [](char c) { return !isalpha(c); });
        comparisonKey.erase(new_end, comparisonKey.end());

        if (typeName == "float")
        {
            PARSE(float, comparisonType)
        }
        if (typeName == "double")
        {
            PARSE(double, comparisonType)
        }
        if (typeName == "bool")
        {
            PARSE(bool, comparisonType)
        }
        if (typeName == "int")
        {
            PARSE(int, comparisonType)
        }
        if (typeName.find("string") != std::string::npos)
        {
            PARSE(std::string, comparisonType)
        }
    }
    if (name == "Action")
    {
        std::string actionName = json["type"];
        DeserializeAction(actionName, json["editor-variables"],builder);
    }

    for (int i = 0; i < numChildren; i++)
    {
        DeserializeBehavior(children[i],builder);
    }

    if (builder.GetNodeStack().empty()) return;
    builder.Back();
}

void fluczakAI::BehaviorTree::Deserialize(nlohmann::json& json)
{
    std::string version = json["version"];
    BehaviorTreeBuilder builder;
    if (json["behavior-structure-type"].get<std::string>() != "BehaviorTree") return {};
    if (!json.contains("children")) return {};
    const auto toDeserialize = json.at("children")[0];
    DeserializeBehavior(toDeserialize,builder);
    const std::unique_ptr<BehaviorTree> newTree = builder.End();
    m_root.swap(newTree->GetRoot());
}
#endif


fluczakAI::BehaviorTree::BehaviorTree(std::unique_ptr<Behavior>& rootToSet)
{
    m_root.swap(rootToSet);
}
