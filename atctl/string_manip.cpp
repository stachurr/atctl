#include "string_manip.h"
#include "../common.h"



static std::vector<std::string> _split (
    std::string &str,
    const std::string &delimeter,
    bool keep_delimeter,
    unsigned int beg_idx,
    int end_idx
){
    DBG("Separating string: %s\n", str.c_str());
    std::vector<std::string> parts;



    // Separate string into parts by delimeter.
    size_t beg = 0;
    while (1)
    {
        size_t pos = str.find(delimeter, beg);
        if (pos != std::string::npos)
        {
            size_t end = pos - beg;
            if (keep_delimeter)
            {
                end++;
            }

            parts.emplace_back(str.substr(beg, end));
            beg = pos + 1;
        }
        else
        {
            if (beg < str.length() - 1)
            {
                parts.emplace_back(str.substr(beg));
            }

            break;
        }
    }

    // Remove unwanted parts.
    if (end_idx != 0)
    {
        parts.erase(end_idx > 0
                        ? std::min(parts.begin() + end_idx, parts.end())    // (end_idx is non-negative) do not erase passed end
                        : std::max(parts.end() + end_idx, parts.begin()),   // (end_idx is negative) do not erase before begin
                    parts.end());
    }

    if (beg_idx > 0)
    {
        parts.erase(parts.begin(),
                    parts.begin() + std::min(static_cast<size_t>(beg_idx), parts.size()));
    }



    return parts;
}

std::vector<std::string> split(
    std::string &str,
    const std::string &delimeter /*= SPLIT_DEFAULT_DELIM*/,
    bool keep_delimeter /*= SPLIT_DEFAULT_KEEP_DELIM*/,
    unsigned int beg_idx /*= SPLIT_DEFAULT_BEG_IDX*/,
    int end_idx /*= SPLIT_DEFAULT_END_IDX*/
){
    return _split(str, delimeter, keep_delimeter, beg_idx, end_idx);
}

std::vector<std::string> split (
    std::string &str,
    unsigned int beg_idx,
    int end_idx /*= SPLIT_DEFAULT_END_IDX*/
){
    return _split(str, SPLIT_DEFAULT_DELIM, SPLIT_DEFAULT_KEEP_DELIM, beg_idx, end_idx);
}



std::string& strip (std::string &str)
{
    while (!str.empty() && std::isspace(str.front()))
    {
        str.erase(0, 1);
    }

    while (!str.empty() && std::isspace(str.back()))
    {
        str.pop_back();
    }

    return str;
}


