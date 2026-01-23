#pragma once

#include <algorithm>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "FileUtils.h"
#include "Types.h"

void default_gw2key_to_skillslot_mapping(const Keys gw2_key, SkillSlot &detected_skill_slot, std::string &detected_action_name);

ProfessionID string_to_profession(const std::string &profession_name);

EliteSpecID string_to_elite_spec(const std::string &spec_name);

std::string profession_to_string(ProfessionID profession_id);

std::string elite_spec_to_string(EliteSpecID elite_spec_id);

std::vector<std::string> get_elite_specs_for_profession(ProfessionID profession);

std::string default_skillslot_to_string(SkillSlot skill_type);

SkillSlot str_to_default_skillslot(const std::string keybind_str);

std::string custom_keys_to_string(Keys key);

std::string modifiers_to_string(Modifiers modifier);

std::pair<Keys, Modifiers> get_keybind_for_skill_type(SkillSlot skill_type,
                                                      const std::map<std::string, KeybindInfo> &keybinds);

std::string download_state_to_string(DownloadState state);

std::string weapon_type_to_string(WeaponType weapon_type);

SkillID SafeConvertToSkillID(uint64_t skill_id_raw);

std::string windows_key_to_string(WindowsKeys key);

Keys windows_key_to_keys_enum(WindowsKeys key);
