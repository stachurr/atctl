#pragma once

#include <source_location>
#include <filesystem>

#ifdef _WIN32
    #define _consteval  constexpr
#else
    #define _consteval  consteval
#endif



// A value wrapper which provides source code information.
template<typename WrappedValueType>
struct source_location_wrapper
{
    _consteval source_location_wrapper (WrappedValueType value, const std::source_location& loc = std::source_location::current())
        : m_wrapped_value   (value)
        , m_loc             (loc)
    {}

    constexpr WrappedValueType get_wrapped (void) const noexcept
    {
        return m_wrapped_value;
    }

    constexpr uint_least32_t line (void) const noexcept
    {
        return m_loc.line();
    }

    constexpr uint_least32_t column (void) const noexcept
    {
        return m_loc.column();
    }

    constexpr const char* file_name (void) const noexcept
    {
        return m_loc.file_name();
    }

    constexpr const char* function_name (void) const noexcept
    {
        return m_loc.function_name();
    }

    constexpr const char* file_shortname (void) const noexcept
    {
        return this->get_shortname(this->file_name());
    }



private:
    WrappedValueType            m_wrapped_value;
    const std::source_location  m_loc;

    static constexpr const char* get_shortname (const char *full_path)
    {
        const char *shortname = full_path;

        while (*shortname)
        {
            shortname++;
        }

        while (*shortname != std::filesystem::path::preferred_separator && shortname > full_path)
        {
            shortname--;
        }

        return shortname == full_path ? shortname : shortname + 1;
    }
};



using source_location_wrapper_cstring = source_location_wrapper<const char*>;

