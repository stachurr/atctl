#pragma once

#include "source_location_wrapper.h"




class source_exception : protected source_location_wrapper_cstring
{
public:
    _consteval source_exception (const char *what, const std::source_location& loc = std::source_location::current())
        : source_location_wrapper_cstring (what, loc)
        , m_what (nullptr)
    {}

    const char* what (void) const;


private:
    mutable std::unique_ptr<std::string>    m_what;
};

