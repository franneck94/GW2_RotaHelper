#pragma once

#include <string>
#include <map>

#include "Types.h"

class BuildsType
{
public:
    void initialize_build_categories();
    BuildCategory get_build_category(const std::string &display_name) const;

    bool build_categories_initialized = false;
    std::map<std::string, BuildCategory> build_category_cache;
};
