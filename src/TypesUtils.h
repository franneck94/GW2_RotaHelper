#pragma once

#include <algorithm>
#include <string>
#include <vector>

#include "Types.h"

ProfessionID string_to_profession(const std::string &profession_name);

EliteSpecID string_to_elite_spec(const std::string &spec_name);

std::string profession_to_string(ProfessionID profession_id);

std::string elite_spec_to_string(EliteSpecID elite_spec_id);

std::vector<std::string> get_elite_specs_for_profession(
    ProfessionID profession);

std::string get_keybind_str(SkillSlot skill_type);

SkillSlot load_keybind(const std::string keybind_str);

std::string keys_to_string(Keys key);

std::string modifiers_to_string(Modifiers modifier);
