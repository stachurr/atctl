#include "source_exception.h"


const char* source_exception::what (void) const
{
    if (!m_what)
    {
        m_what.reset(new std::string());
        m_what->append(this->get_wrapped()).append(" -- ").append(this->file_shortname()).append(":").append(std::to_string(this->line()));
    }

    return m_what->c_str();
}
