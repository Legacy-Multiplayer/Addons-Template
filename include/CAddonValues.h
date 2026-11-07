#pragma once

#include "Addon.h"

#include <algorithm>
#include <cmath>
#include <deque>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

// All typed bindings use the same checked conversions. Values are never silently
// truncated to IDs or converted from strings to numbers.
class CAddonValues
{
    template <typename T>
    struct Optional : std::false_type
    {
    };
    template <typename T>
    struct Optional<std::optional<T>> : std::true_type
    {
        using Type = T;
    };

public:
    template <typename T>
    static std::decay_t<T> Read(const Value* values, size_t count, size_t index)
    {
        using Type = std::decay_t<T>;
        if constexpr (Optional<Type>::value)
        {
            if (!values || index >= count || values[index].type == ADDON_VALUE_NIL)
            {
                return std::nullopt;
            }
            return Read<typename Optional<Type>::Type>(values, count, index);
        }
        else
        {
            if (!values || index >= count)
            {
                throw std::invalid_argument("Missing argument " + std::to_string(index + 1));
            }
            const auto& value = values[index];
            if constexpr (std::is_same_v<Type, bool>)
            {
                if (value.type == ADDON_VALUE_BOOLEAN)
                {
                    return value.booleanValue;
                }
            }
            else if constexpr (std::is_integral_v<Type>)
            {
                int64_t integer = 0;
                if (value.type == ADDON_VALUE_INTEGER)
                {
                    integer = value.integerValue;
                }
                else if (value.type == ADDON_VALUE_NUMBER && std::isfinite(value.numberValue) &&
                         std::trunc(value.numberValue) == value.numberValue &&
                         value.numberValue >= -9223372036854775808.0 && value.numberValue < 9223372036854775808.0)
                {
                    integer = static_cast<int64_t>(value.numberValue);
                }
                else
                {
                    throw std::invalid_argument("Expected an integer at argument " + std::to_string(index + 1));
                }
                if constexpr (std::is_unsigned_v<Type>)
                {
                    if (integer < 0 || static_cast<uint64_t>(integer) > std::numeric_limits<Type>::max())
                    {
                        throw std::out_of_range("Unsigned integer argument out of range");
                    }
                }
                else if (integer < std::numeric_limits<Type>::min() || integer > std::numeric_limits<Type>::max())
                {
                    throw std::out_of_range("Integer argument out of range");
                }
                return static_cast<Type>(integer);
            }
            else if constexpr (std::is_floating_point_v<Type>)
            {
                if (value.type != ADDON_VALUE_INTEGER && value.type != ADDON_VALUE_NUMBER)
                {
                    throw std::invalid_argument("Expected a number");
                }
                const auto number = value.type == ADDON_VALUE_INTEGER ? static_cast<double>(value.integerValue) : value.numberValue;
                if ((value.type == ADDON_VALUE_INTEGER || value.type == ADDON_VALUE_NUMBER) &&
                    std::isfinite(number) && number >= -std::numeric_limits<Type>::max() && number <= std::numeric_limits<Type>::max())
                {
                    return static_cast<Type>(number);
                }
            }
            else if constexpr (std::is_same_v<Type, const char*>)
            {
                if (value.type == ADDON_VALUE_STRING && value.stringValue)
                {
                    return value.stringValue;
                }
            }
            else if constexpr (std::is_same_v<Type, std::string>)
            {
                if (value.type == ADDON_VALUE_STRING && value.stringValue)
                {
                    return std::string(value.stringValue);
                }
            }
            else
            {
                static_assert(!sizeof(Type), "Unsupported addon argument type; use bool, integer, float, double, string or optional");
            }
            if constexpr (!std::is_integral_v<Type> || std::is_same_v<Type, bool>)
            {
                throw std::invalid_argument("Wrong type at argument " + std::to_string(index + 1));
            }
        }
    }

    template <typename T>
    static Value Write(const T& value, std::deque<std::string>& strings)
    {
        using Type = std::decay_t<T>;
        if constexpr (Optional<Type>::value)
        {
            return value ? Write(*value, strings) : ValueNil();
        }
        else if constexpr (std::is_same_v<Type, bool>)
        {
            return ValueBoolean(value);
        }
        else if constexpr (std::is_integral_v<Type>)
        {
            if constexpr (std::is_unsigned_v<Type>)
            {
                if (value > static_cast<uint64_t>(std::numeric_limits<int64_t>::max()))
                {
                    throw std::out_of_range("Lua integer result out of range");
                }
            }
            return ValueInteger(static_cast<int64_t>(value));
        }
        else if constexpr (std::is_floating_point_v<Type>)
        {
            if (!std::isfinite(value))
            {
                throw std::invalid_argument("Non-finite addon result");
            }
            return ValueNumber(value);
        }
        else if constexpr (std::is_convertible_v<Type, std::string>)
        {
            strings.emplace_back(value);
            return ValueString(strings.back().c_str());
        }
        else
        {
            static_assert(!sizeof(Type), "Unsupported addon result type");
        }
    }
};

template <typename T>
struct CAddonCallable : CAddonCallable<decltype(&T::operator())>
{
};
template <typename R, typename... Args>
struct CAddonCallable<R (*)(Args...)>
{
    using Arguments = std::tuple<Args...>;
    static constexpr size_t Count = sizeof...(Args);
};
template <typename C, typename R, typename... Args>
struct CAddonCallable<R (C::*)(Args...) const> : CAddonCallable<R (*)(Args...)>
{
};
template <typename C, typename R, typename... Args>
struct CAddonCallable<R (C::*)(Args...)> : CAddonCallable<R (*)(Args...)>
{
};

template <typename Function, size_t... I>
static decltype(auto) InvokeAddon(Function& function, const Value* args, size_t count, std::index_sequence<I...>)
{
    using Arguments = typename CAddonCallable<Function>::Arguments;
    return function(CAddonValues::Read<std::tuple_element_t<I, Arguments>>(args, count, I)...);
}

class CAddonResults
{
public:
    template <typename T>
    int Store(T&& result, Value* output, size_t capacity)
    {
        m_strings.clear();
        m_values.clear();
        Append(std::forward<T>(result));
        if (!output || m_values.size() > capacity)
        {
            throw std::length_error("Too many addon results");
        }
        std::copy(m_values.begin(), m_values.end(), output);
        return static_cast<int>(m_values.size());
    }

private:
    template <typename T>
    void Append(const T& value)
    {
        m_values.push_back(CAddonValues::Write(value, m_strings));
    }
    template <typename... T>
    void Append(const std::tuple<T...>& values)
    {
        std::apply([this](const auto&... value)
            {
                (Append(value), ...);
            },
            values);
    }
    std::deque<std::string> m_strings;
    std::vector<Value> m_values;
};
