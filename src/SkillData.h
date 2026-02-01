#pragma once

#include <map>
#include <set>
#include <string>
#include <string_view>

#include "Types.h"

namespace SkillRuleData
{

extern const std::set<SkillID> skills_to_not_track;

extern const std::map<SkillID, SkillID> special_mapping_skills;

extern const std::set<SkillID> berserker_f1_skills;

extern const std::set<SkillID> mesmer_weapon_4_skills;

extern const std::set<SkillID> reset_like_skill;

extern const std::map<SkillID, float> skill_cast_time_map;

extern const std::map<SkillID, float> grey_skill_cast_time_map;

extern const SkillRules skill_rules;

extern const std::map<int, int> unk_skill_id_fix;

extern const std::map<int, int> unk_skill_id_based_on_icon_id_fix;

extern const std::map<int, int> fix_skill_img_ids;

SkillData GetDataByID(const SkillID skill_id, const SkillDataMap &skill_data_map);

bool IsProfessionResetLikeSKill(const SkillID skill_id);

} // namespace SkillRuleData
