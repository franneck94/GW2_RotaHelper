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

extern const std::set<std::string_view> special_match_to_gray_out_names;

extern const std::set<SkillID> special_match_to_gray_out;

extern const std::set<std::string_view> special_substr_to_remove_duplicates_names;

extern const std::set<SkillID> special_substr_to_remove_duplicates;

extern const std::set<std::string_view> easy_mode_drop_match_name;

extern const std::set<SkillID> easy_mode_drop_match;

extern const std::set<SkillID> skills_to_not_track;

extern const std::map<SkillID, SkillID> special_mapping_skills;

extern const std::set<SkillID> berserker_f1_skills;

extern const std::set<SkillID> mesmer_weapon_4_skills;

extern const std::set<SkillID> reset_like_skill;

extern const std::map<SkillID, float> skill_cast_time_map;

extern const std::map<SkillID, float> grey_skill_cast_time_map;

extern const std::map<std::string_view, std::set<SkillID>> class_map_special_match_to_gray_out;

extern const std::map<std::string_view, std::set<SkillID>> class_map_special_match_to_gray_out;

extern const SkillRules skill_rules;

SkillData GetDataByID(const SkillID skill_id, const SkillDataMap &skill_data_map);

bool IsProfessionResetLikeSKill(const SkillID skill_id);

} // namespace SkillRuleData
