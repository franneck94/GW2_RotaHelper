#pragma once

#include <map>
#include <set>
#include <string>
#include <string_view>

#include "Types.h"

namespace SkillRuleData
{

extern const std::set<std::string_view> skills_substr_weapon_swap_like;

extern const std::set<std::string_view> skills_match_weapon_swap_like;

extern const std::set<std::string_view> skills_substr_to_drop;

extern const std::set<std::string_view> skills_match_to_drop;

extern const std::set<std::string_view> special_substr_to_gray_out;

extern const std::set<std::string_view> special_match_to_gray_out;

extern const std::set<std::string_view> special_substr_to_remove_duplicates;

extern const std::set<std::string_view> easy_mode_drop_match;

extern const SkillRules skill_rules;

extern const std::set<std::string> skills_to_not_track;

extern const std::map<std::string, std::string> special_mapping_skills;

extern const std::set<uint64_t> berserker_f1_skills;

extern const std::set<uint64_t> mesmer_weapon_4_skills;

extern const std::set<uint64_t> reset_like_skill;

extern const std::map<std::string_view, float> skill_cast_time_map;

} // namespace SkillData
