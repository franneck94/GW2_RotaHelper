#pragma once

#include <algorithm>
#include <map>
#include <string>
#include <vector>

#include "Types.h"

ProfessionID string_to_profession(const std::string &profession_name);

EliteSpecID string_to_elite_spec(const std::string &spec_name);

std::string profession_to_string(ProfessionID profession_id);

std::string elite_spec_to_string(EliteSpecID elite_spec_id);

std::vector<std::string> get_elite_specs_for_profession(
    ProfessionID profession);

std::string skillslot_to_string(SkillSlot skill_type);

SkillSlot str_to_skillslot(const std::string keybind_str);

std::string custom_keys_to_string(Keys key);

std::string modifiers_to_string(Modifiers modifier);

Keys get_keybind_for_skill_type(SkillSlot skill_type,
                                const std::map<std::string, KeybindInfo>& keybinds);
