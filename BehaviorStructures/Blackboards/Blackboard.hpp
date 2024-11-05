#pragma once

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <cassert>
#include <sstream>


namespace fluczakAI
{
    template <typename T>
    struct HasToStringBlackboard
    {
        template <typename U>
        static auto Test(U*) -> decltype(std::to_string(std::declval<U>()), std::true_type{});

        template <typename U>
        static auto Test(...) -> std::false_type;

        static constexpr bool value = decltype(Test<T>(0))::value;
    };

    template <typename T>
    struct HasIteratorsBlackboard
    {
    private:
        template <typename U>
        static auto Test(U*) -> decltype(std::begin(std::declval<U>()), std::end(std::declval<U>()), std::true_type{});

        template <typename U>
        static auto Test(...) -> std::false_type;

    public:
        static constexpr bool value = decltype(Test<T>(0))::value;
    };

    class Blackboard
    {
    public:
        /**
         * \brief Sets the data at given key to a given value of TVAlueType. If the value is already there, replace it with
         * a new value.
         * \tparam TValueType - the type of the value that is going to be set
         * \param key - the key where the value is located in the blackboard.
         * \param value - the value of type TValueType
         */
        template <typename TValueType>
        void SetData(const std::string& key, TValueType value)
        {
            assert(!std::is_reference<TValueType>());

            if (m_Map.find(key) != m_Map.end())
            {
                static_cast<Handle<TValueType>*>(m_Map[key].get())->SetData(value);
            }
            else
            {
                m_Map[key] = std::make_unique<Handle<TValueType>>(Handle<TValueType>(std::move(value)));
            }
        }

        /**
         * \brief Get reference to an element of a key 'key'
         * \tparam TValueType - The type of value that is set for the key
         * \param key - key of the value
         * \return - a reference to the element inside the blackboard.
         */
        template <typename TValueType>
        TValueType& GetData(const std::string& key) const
        {
            const auto it = m_Map.find(key);

            assert(it != m_Map.end());

            auto* handle = dynamic_cast<Handle<TValueType>*>(it->second.get());

            // TValueType needs to be of the same type as the value in the blackboard
            assert(handle != nullptr);


            return *handle->get();
        }

        /**
         * \brief Try get returns a pointer to the given element. If it doesn't exist
         * a nullptr is returned
         * \tparam TValueType - The type of value that is set for the key
         * \param key - key of the value
         * \return - a pointer to element or nullptr
         */
        template <typename TValueType>
        TValueType* const TryGet(const std::string& key) const
        {
            const auto it = m_Map.find(key);

            if (it == m_Map.end()) return nullptr;

            auto* handle = dynamic_cast<Handle<TValueType>*>(it->second.get());

            if (handle == nullptr) return nullptr;

            return handle->get();
        }

        /**
         * \brief A check method if a given key has a value set
         * \param key - the key to check
         * \return - whether or not the key has a value set inside the blackboard
         */
        template <typename TValueType>
        bool HasKey(const std::string& key) const
        {
            const auto it = m_Map.find(key);

            if (it == m_Map.end()) return false;

            auto* handle = dynamic_cast<Handle<TValueType>*>(it->second.get());

            if (handle == nullptr) return false;

            return true;
        }

        /**
         * \brief Clear all of the values of the blackboard
         */
        void Clear()
        {
            m_Map.clear();
        }

        std::vector<std::pair<std::string, std::string>> PreviewToString();
    private:
        struct IHandle
        {
            virtual ~IHandle() = default;
        };

        template <typename T>
        struct Handle : IHandle
        {
            Handle(T data) : m_Data(std::move(data)) {}
            T* get() { return &(m_Data); }
        	void SetData(T& t) { m_Data = t; }
        private:
            T m_Data;
        };

        std::unordered_map<std::string, std::unique_ptr<IHandle>> m_Map;
    };
}
