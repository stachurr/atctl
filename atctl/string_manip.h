#pragma once

#include <vector>
#include <string>

static constexpr const char    *SPLIT_DEFAULT_DELIM         = "\n";
static constexpr bool           SPLIT_DEFAULT_KEEP_DELIM    = false;
static constexpr unsigned int   SPLIT_DEFAULT_BEG_IDX       = 0;
static constexpr int            SPLIT_DEFAULT_END_IDX       = 0;




std::vector<std::string> split(
    std::string &str,
    const std::string &delimeter = SPLIT_DEFAULT_DELIM,
    bool keep_delimeter = SPLIT_DEFAULT_KEEP_DELIM,
    unsigned int beg_idx = SPLIT_DEFAULT_BEG_IDX,
    int end_idx = SPLIT_DEFAULT_END_IDX
);

std::vector<std::string> split (
    std::string &str,
    unsigned int beg_idx,
    int end_idx = SPLIT_DEFAULT_END_IDX
);



std::string& strip (std::string &str);
