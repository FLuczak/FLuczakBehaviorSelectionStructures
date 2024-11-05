#pragma once
#include <filesystem>
#include <sstream>
#include <string>
#include <typeindex>

#include "../BehaviorTrees/behavior_tree.hpp"
#include "../FSM/finite_state_machine.hpp"
#include "magic_enum/include/magic_enum/magic_enum.hpp"
#include "visit_struct/include/visit_struct/visit_struct.hpp"

namespace fluczakAI
{

struct PathHelper
{
    std::filesystem::path path{};
    std::string format{};
};

	template <typename U>
concept HasToString = requires(const U & type)
{
    { std::to_string(type) } -> std::convertible_to<std::string>;
};

template <typename U>
concept HasIterators = requires(const U & type)
{
    type.begin();
    type.end();
};

template <typename U, typename  T>
concept HasPushBack = requires(U u, T t)
{
    { u.push_back(t) } -> std::same_as<void>;
    { u.push_back(std::move(t)) } -> std::same_as<void>;
};

template <typename U, typename  T>
concept HasInsert = requires(U u, T t)
{
    { u.insert(t) } -> std::same_as<void>;
    { u.insert(std::move(t)) } -> std::same_as<void>;
};


class EditorVariable
{
public:
    virtual ~EditorVariable() = default;
    virtual std::string ToString() const = 0;
    virtual std::type_index GetTypeInfo() const = 0;
    virtual void Deserialize(const std::string& serializedValue) = 0;
    virtual std::vector<std::string> GetEnumNames() = 0;
    virtual std::type_index GetUnderlyingType() const = 0;
};


template <typename T>
class SerializedField : public EditorVariable
{
public:
    SerializedField(T& variable) :value(variable) {};

    SerializedField(fluczakAI::BehaviorTreeAction& action, const std::string& name, T& variable) : value(variable)
    {
        action.RegisterEditorVariable(name, this);
    }

   SerializedField(fluczakAI::State& state, const std::string& name, T& variable) : value(variable)
   {
       state.RegisterEditorVariable(name, this);
   }

    T GetValue() { return value; }

    /// @brief Converts a given value to a string representation using specialized handling for certain types.
	/// @param val The value to be converted to a string.
	/// @return A string representation of the input value.
    std::string ValueToString(T& val)const
    {
        if constexpr (std::is_enum_v<T>)
        {
            return std::string(magic_enum::enum_name(val));
        }

        if constexpr (std::is_same<T, std::string>::value)
        {
            return val;
        }

        if constexpr (HasToString<T>)
        {
            return std::to_string(val);
        }

        if constexpr (visit_struct::traits::is_visitable<T>::value)
        {
            std::stringstream ss;
            visit_struct::for_each(val, [&ss](const char* name, const auto& value)
                {
                    ss << name << ":" << value << ' ';
                });
            return ss.str();
        }

        if constexpr (HasIterators<T>)
        {
            std::stringstream ss;
            for (auto element : val)
            {
                SerializedField<decltype(element)> f(element);
                ss << "{" << f.ToString() << "}";
            }
            return ss.str();
        }

        if constexpr (std::is_same_v<T,PathHelper>)
        {
            auto helper = static_cast<PathHelper>(val);
            nlohmann::json j;
            j["format"] = helper.format;
            j["path"] = helper.path.string();
            return j.dump();
        }

        return "";
    }

    std::string ToString() const override
    {
        return ValueToString(value);
    }

    T DeserializeVisitableStruct( std::stringstream& stringStream)
    {
        T toReturn{};
	    visit_struct::for_each(toReturn, [&stringStream](const char* name, auto& value)
	    {
		    std::string tempValue;
		    stringStream >> tempValue;
		    size_t colonPos = tempValue.find(':');

		    std::string stringName = tempValue.substr(0, colonPos);
		    std::stringstream val = std::stringstream(tempValue.substr(colonPos + 1));

		    val >> value;
	    });
	    return toReturn;
    }

    T DeserializeEnum(const std::stringstream& stringStream)
    {
	    T toReturn;
	    auto temp = magic_enum::enum_cast<T>(stringStream.str());
	    if (temp.has_value())
	    {
		    toReturn = temp.value();
	    }
	    return toReturn;
    }

    T DeserializeCollection(std::stringstream& stringStream)
    {
	    T toReturn;
	    std::string element;
	    while (std::getline(stringStream, element, '}'))
	    {
		    // Extract the content inside curly braces
		    std::string content = element.substr(1, element.size() - 1);

		    using ElementType = typename std::remove_const<std::remove_reference<decltype(*std::begin(std::declval<T>()))>::type>::type;
		    ElementType d;
		    SerializedField<ElementType> temp(d);
		    temp.Deserialize(content);

		    if constexpr (HasPushBack<T, ElementType>)
		    {
			    toReturn.push_back(temp.GetValue());
		    }
		    else if constexpr (HasInsert<T, ElementType> && !HasPushBack<T, ElementType>)
		    {
			    toReturn.insert(temp.GetValue());
		    }
	    }
	    return toReturn;
    }

    T DeserializePathHelper(std::stringstream& stringStream)
    {
	    nlohmann::json j = nlohmann::json::parse(stringStream.str());
	    PathHelper helper;
	    helper.format = j["format"].get<std::string>();
	    helper.path = std::filesystem::path(j["path"].get<std::string>());

	    return helper;
    }

    /// @brief Deserializes a value from a string representation.
	/// @param string The string containing the serialized value.
	/// @return The deserialized value of type `T`.
    T GetDeserializedValue(const std::string& string)
    {
        T toReturn;
        std::istringstream stringStream(string);

        if constexpr (visit_struct::traits::is_visitable<T>::value)
        {
            return DeserializeVisitableStruct(toReturn, stringStream);
        }

        if constexpr (std::is_same<T, std::string>::value)
        {
            stringStream >> toReturn;
            return toReturn;
        }

        if constexpr (std::is_enum_v<T>)
        {
            return DeserializeEnum(stringStream);
        }

        if constexpr (HasToString<T>)
        {
            stringStream >> toReturn;
            return toReturn;
        }

        if constexpr (HasIterators<T> && !std::is_same<T, std::string>::value)
        {
            return DeserializeCollection(stringStream);
        }

        if constexpr (std::is_same<T, PathHelper>::value)
        {
            return DeserializePathHelper(stringStream);
        }

        return toReturn;
    }

    void Deserialize(const std::string& serializedValue) override
    {
        value = GetDeserializedValue(serializedValue);
    }

    /// @brief Extracts individual elements from a serialized collection
	/// @param input The input string containing elements enclosed in braces.
	/// @return A vector of strings, each representing an extracted element.
    std::vector<std::string> ExtractElements(const std::string& input)
    {
        std::vector<std::string> elements;
        std::string element;

        bool insideBraces = false;
        for (char c : input)
        {
            if (c == '{')
            {
                insideBraces = true;
            }
            else if (c == '}')
            {
                insideBraces = false;
                if (!element.empty())
                {
                    elements.push_back(element);
                    element.clear();
                }
            }
            else if (insideBraces)
            {
                element += c;
            }
        }

        return elements;
    }

    /// @brief Retrieves type information of the stored value.
	/// @return A `std::type_index` representing the type of the stored value
    std::type_index GetTypeInfo() const override
    {
        return typeid(T);
    }

    /// @brief Retrieves the names of enumeration values if the stored type is an enum.
	/// @return A vector of strings containing the names of enum values.
    std::vector<std::string> GetEnumNames() override
    {
        assert(std::is_enum_v<T>);
        std::vector<std::string> toReturn;
        if constexpr (std::is_enum_v<T>)
        {
            constexpr auto names = magic_enum::enum_names<T>();
            for (auto name : names)
            {
                toReturn.emplace_back(name);
            }
            return toReturn;
        }

        return {};
    }

    /// @brief Gets the type information of the underlying type of a collection if the stored type is iterable.
	/// @return A `std::type_index` representing the type of the elements in the stored type if iterable, or `void*` otherwise.
    std::type_index GetUnderlyingType() const override
    {
        if constexpr (HasIterators<T>)
        {
            return typeid(*value.begin());
        }
        return typeid(void*);
    }

private:
    T& value;
};
}

/// @brief Macro to define a serializable field with automatic registration.
/// @param type The data type of the field.
/// @param name The name of the field.
#define SERIALIZE_FIELD(type, name) \
        type name;                             \
        SerializedField<type> name##Helper{*this, #name, name};

/// @brief Macro to define a serializable `PathHelper` field with a specified format and automatic registration.
/// @param format The format string to associate with the `PathHelper`.
/// @param name The name of the `PathHelper` field.
#define SERIALIZE_FILE_PATH(format, name) \
        PathHelper name{"",#format};                             \
        SerializedField<PathHelper> name##Helper{*this, #name, name};