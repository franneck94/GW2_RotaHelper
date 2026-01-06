#include <windows.h>

#include <commdlg.h>
#include <wininet.h>
#pragma comment(lib, "wininet.lib")
#include <wincodec.h>
#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "ole32.lib")
#include <conio.h>
#include <d3d11.h>

#include <algorithm>
#include <d3d11.h>
#include <filesystem>
#include <fstream>
#include <future>
#include <iostream>
#include <list>
#include <map>
#include <string>
#include <vector>

#include "nlohmann/json.hpp"

#include "FileUtils.h"
#include "LogData.h"
#include "MumbleUtils.h"
#include "Settings.h"
#include "Shared.h"
#include "SkillData.h"
#include "Types.h"
#include "TypesUtils.h"

using json = nlohmann::json;


namespace
{
void collect_json(const json &jval, IntNode &node, const bool drop_first_char = false)
{
    if (jval.is_object())
    {
        for (auto it = jval.begin(); it != jval.end(); ++it)
        {
            auto key = std::string{it.key()};
            if (drop_first_char && !key.empty())
            {
                key = key.substr(1);
            }
            collect_json(it.value(), node.children[key]);
        }
    }
    else if (jval.is_array())
    {
        for (size_t i = 0; i < jval.size(); ++i)
        {
            collect_json(jval[i], node.children[std::to_string(i)]);
        }
    }
    else if (jval.is_number_integer())
    {
        node.value = jval.get<int>();
    }
    else if (jval.is_boolean())
    {
        node.value = jval.get<bool>();
    }
    else if (jval.is_number_float())
    {
        node.value = jval.get<float>();
    }
    else if (jval.is_string())
    {
        node.value = jval.get<std::string>();
    }
}

std::string get_cache_remainder(const std::string &cache_url, const std::string &cache)
{
    const auto remainder = cache_url.substr(cache.length());

    return remainder;
}

std::string convert_cache_url(const std::string &cache_url)
{
    static const auto cache1 = std::string{"/cache/https_render.guildwars2.com_file_"};
    static const auto cache2 = std::string{"/cache/https_wiki.guildwars2.com_images_"};

    if (cache_url.find(cache1) != 0 && cache_url.find(cache2))
        return cache_url;

    auto remainder = std::string{};
    if (cache_url.find(cache1))
        remainder = get_cache_remainder(cache_url, cache1);
    else if (cache_url.find(cache2))
        remainder = get_cache_remainder(cache_url, cache2);
    else
        return cache_url;

    const auto last_underscore = remainder.find_last_of('_');
    if (last_underscore == std::string::npos)
        return cache_url;

    const auto hash = remainder.substr(0, last_underscore);
    const auto icon_id_with_ext = remainder.substr(last_underscore + 1);

    const auto dot_pos = icon_id_with_ext.find_last_of('.');
    const auto icon_id = (dot_pos != std::string::npos) ? icon_id_with_ext.substr(0, dot_pos) : icon_id_with_ext;

    return "https://render.guildwars2.com/file/" + hash + "/" + icon_id + ".png";
}


void get_skill_info(const IntNode &node, LogSkillInfoMap &log_skill_info_map)
{
    for (const auto &[icon_id, skill_node] : node.children)
    {
        auto name = std::string{};
        auto icon = std::string{};

        const auto name_it = skill_node.children.find("name");
        if (name_it != skill_node.children.end() && name_it->second.value.has_value())
        {
            if (const auto pval = std::get_if<std::string>(&name_it->second.value.value()))
                name = *pval;
        }

        const auto icon_it = skill_node.children.find("icon");
        if (icon_it != skill_node.children.end() && icon_it->second.value.has_value())
        {
            if (const auto pval = std::get_if<std::string>(&icon_it->second.value.value()))
                icon = convert_cache_url(*pval);
        }

        log_skill_info_map[std::stoi(icon_id)] = {
            name,
            icon,
        };
    }

    log_skill_info_map[-9999] = {
        "Unknown Skill",
        "local://-9999.png",
    };

    log_skill_info_map[2] = {
        "Dodge",
        "https://wiki.guildwars2.com/images/b/b2/Dodge.png",
    };
}


bool remove_skill_if(const RotationStep &current, const RotationStep &previous)
{
    return (current.skill_data.skill_id == previous.skill_data.skill_id &&
            (current.time_of_cast - previous.time_of_cast) < 250);
}

bool get_is_skill_dropped(const SkillData &skill_data,
                          const SkillRules &skill_rules,
                          const bool is_strict_mode,
                          const bool show_weapon_swap,
                          const bool is_easy_skill_mode)
{
    const auto is_substr_drop_match = is_skill_in_set(skill_data.name, skill_rules.skills_substr_to_drop);
    const auto is_exact_drop_match = is_skill_in_set(skill_data.name, skill_rules.skills_match_to_drop, true);

    auto drop_skill = is_substr_drop_match || is_exact_drop_match;
    if (!show_weapon_swap || is_strict_mode)
    {
        const auto drop_substr_swap = is_skill_in_set(skill_data.name, skill_rules.skills_substr_weapon_swap_like);
        const auto drop_match_swap = is_skill_in_set(skill_data.name, skill_rules.skills_match_weapon_swap_like, true);

        const auto is_unknownm = skill_data.name.find("Unknown Skill") != std::string::npos;

        drop_skill = is_substr_drop_match || is_exact_drop_match || drop_substr_swap || drop_match_swap || is_unknownm;

        if (!drop_skill)
        {
            auto current_spec = get_current_spec_name();
            auto spec_lower = to_lowercase(current_spec);

            if ((spec_lower == "vindicator" || spec_lower == "mirage" || spec_lower == "druid") &&
                skill_data.name == "Dodge")
                drop_skill = false;
        }

        drop_skill = is_substr_drop_match || is_exact_drop_match || drop_substr_swap || drop_match_swap ||
                     is_unknownm || drop_skill;
    }

    if (is_easy_skill_mode && !drop_skill)
    {
        auto is_exact_easy_mode_drop_match =
            is_skill_in_set(skill_data.name, skill_rules.easy_mode_drop_match_name, true) ||
            is_skill_in_set(skill_data.skill_id, skill_rules.easy_mode_drop_match);

        const auto stem = Globals::Render.selected_file_path.stem().string();
        if (stem != "")
        {
            const auto class_name = stem;
            const auto class_it = skill_rules.class_map_easy_mode_drop_match.find(class_name);
            if (class_it != skill_rules.class_map_easy_mode_drop_match.end())
            {
                const auto &class_special_set = class_it->second;
                is_exact_easy_mode_drop_match = is_skill_in_set(skill_data.skill_id, class_special_set);
            }
        }

        drop_skill = is_exact_easy_mode_drop_match;
    }

    return drop_skill;
}

bool get_is_special_skill(const SkillData &skill_data, const SkillRules &skill_rules)
{
    if (skill_data.is_heal_skill)
        return true;

    const auto is_substr_gray_out = is_skill_in_set(skill_data.name, skill_rules.special_substr_to_gray_out);
    const auto is_match_gray_out =
        is_skill_in_set(skill_data.name, skill_rules.special_match_to_gray_out_names, true) ||
        is_skill_in_set(skill_data.skill_id, skill_rules.special_match_to_gray_out);

    const auto stem = Globals::Render.selected_file_path.stem().string();
    if (stem != "")
    {
        const auto class_name = stem;
        const auto class_it = skill_rules.class_map_special_match_to_gray_out.find(class_name);
        if (class_it != skill_rules.class_map_special_match_to_gray_out.end())
        {
            const auto &class_special_set = class_it->second;
            const auto is_class_special_gray_out = is_skill_in_set(skill_data.skill_id, class_special_set);
            if (is_class_special_gray_out)
                return true;
        }
    }

    if (is_substr_gray_out || is_match_gray_out)
        return true;

    const auto drop_substr_swap = is_skill_in_set(skill_data.name, skill_rules.skills_substr_weapon_swap_like);
    const auto drop_match_swap = is_skill_in_set(skill_data.name, skill_rules.skills_match_weapon_swap_like, true);

    return drop_substr_swap || drop_match_swap;
}

void get_rotation_info(const IntNode &node,
                       const LogSkillInfoMap &log_skill_info_map,
                       RotationSteps &all_rotation_steps,
                       const SkillDataMap &skill_data_map,
                       const bool is_strict_mode,
                       const bool show_weapon_swap,
                       const bool is_easy_skill_mode)
{
    for (const auto &rotation_entry : node.children)
    {
        const auto &rotation_array = rotation_entry.second;

        for (const auto &skill_entry : rotation_array.children)
        {
            const auto &skill_array = skill_entry.second;

            auto icon_id = 0;
            auto duration_ms = 0.0f;
            auto time_of_cast = 0.0f;

            // Get time_of_cast (index 0)
            const auto time_of_cast_it = skill_array.children.find("0");
            if (time_of_cast_it != skill_array.children.end() && time_of_cast_it->second.value.has_value())
            {
                if (const auto pval = std::get_if<float>(&time_of_cast_it->second.value.value()))
                {
                    time_of_cast = *pval;
                }
            }

            // Get icon_id (index 1)
            const auto icon_id_it = skill_array.children.find("1");
            if (icon_id_it != skill_array.children.end() && icon_id_it->second.value.has_value())
            {
                if (const auto pval = std::get_if<int>(&icon_id_it->second.value.value()))
                {
                    icon_id = *pval;
                }
            }

            // Get duration_ms (index 2)
            const auto duration_it = skill_array.children.find("2");
            if (duration_it != skill_array.children.end() && duration_it->second.value.has_value())
            {
                if (const auto pval = std::get_if<int>(&duration_it->second.value.value()))
                {
                    duration_ms = static_cast<float>(*pval);
                }
            }

            // Search for skill name in skill_data_map using icon_id
            auto skill_data = SkillData{};
            skill_data.recharge_time = -1;
            skill_data.recharge_time_with_alacrity = -1.0f;
            skill_data.cast_time = -1;
            skill_data.cast_time_with_quickness = -1.0f;

            for (const auto &[sid, _skill_data] : skill_data_map)
            {
                if (_skill_data.icon_id == icon_id)
                {
                    skill_data = _skill_data;
                    break;
                }
            }

            // fallback to search icon id in _skill_info_map
            if (skill_data.name == "" || skill_data.skill_id == SkillID::NONE)
            {
                auto skip_skill = false;
                for (const auto &[_icon_id, _skill_data] : log_skill_info_map)
                {
                    if (_icon_id == icon_id)
                    {
                        if (_skill_data.name.find("Relic") != std::string::npos ||
                            _skill_data.name.find("Sigil") != std::string::npos)
                            skip_skill = true;

                        skill_data.skill_id = SafeConvertToSkillID(_icon_id); // TODO : check if we need this if at all
                        skill_data.name = _skill_data.name;
                        skill_data.icon_id = icon_id;

                        if (_skill_data.name.find("Dual") != std::string::npos &&
                            _skill_data.name.find("Attunement") != std::string::npos)
                        {
                            if (_skill_data.name.find("Fire") != std::string::npos)
                                skill_data.skill_id = SkillID::WEAPON_SWAP;
                            else if (_skill_data.name.find("Air") != std::string::npos)
                                skill_data.skill_id = SkillID::WEAPON_SWAP;
                            else if (_skill_data.name.find("Earth") != std::string::npos)
                                skill_data.skill_id = SkillID::WEAPON_SWAP;
                            else if (_skill_data.name.find("Water") != std::string::npos)
                                skill_data.skill_id = SkillID::WEAPON_SWAP;
                            skill_data.name = "Weapon Swap";
                            skill_data.icon_id = (int)SkillID::WEAPON_SWAP;
                        }

                        break;
                    }
                }

                if (skip_skill)
                    continue;
            }

            // TODO: is this always weapon swap?
            if (skill_data.skill_id == SkillID::NONE || skill_data.name == "")
            {
                skill_data.skill_id = SkillID::UNKNOWN_SKILL; // TODO: Check if this works
                skill_data.name = "Weapon Swap";
                skill_data.icon_id = -9999;
            }

            const auto drop_skill = get_is_skill_dropped(skill_data,
                                                         SkillRuleData::skill_rules,
                                                         is_strict_mode,
                                                         show_weapon_swap,
                                                         is_easy_skill_mode);

            if (!drop_skill)
            {
                const auto is_special_skill = get_is_special_skill(skill_data, SkillRuleData::skill_rules);

                const auto cast_time_it = SkillRuleData::skill_cast_time_map.find(skill_data.skill_id);
                if (cast_time_it != SkillRuleData::skill_cast_time_map.end())
                {
                    skill_data.cast_time = cast_time_it->second;
                    skill_data.cast_time_with_quickness = skill_data.cast_time * 0.8f;
                }
                const auto cast_time_it2 = SkillRuleData::grey_skill_cast_time_map.find(skill_data.skill_id);
                if (cast_time_it2 != SkillRuleData::grey_skill_cast_time_map.end())
                {
                    skill_data.cast_time = cast_time_it2->second;
                    skill_data.cast_time_with_quickness = skill_data.cast_time * 0.8f;
                }

                all_rotation_steps.push_back(RotationStep{.time_of_cast = time_of_cast,
                                                          .duration_ms = duration_ms,
                                                          .skill_data = skill_data,
                                                          .is_special_skill = is_special_skill});
            }
        }
    }

    std::sort(all_rotation_steps.begin(), all_rotation_steps.end(), [](const RotationStep &a, const RotationStep &b) {
        return a.time_of_cast < b.time_of_cast;
    });

    for (auto it = all_rotation_steps.begin(); it != all_rotation_steps.end();)
    {
        const auto is_duplicate_skill =
            is_skill_in_set(it->skill_data.name,
                            SkillRuleData::skill_rules.special_substr_to_remove_duplicates_names) ||
            is_skill_in_set(it->skill_data.skill_id, SkillRuleData::skill_rules.special_substr_to_remove_duplicates);

        bool should_remove = false;
        if (is_duplicate_skill && !it->skill_data.is_auto_attack) // && it->skill_data.skill_id != SkillID::DEVASTATOR)
        {
            if (it != all_rotation_steps.begin())
            {
                auto prev_it = std::prev(it);
                should_remove = (prev_it->skill_data.skill_id == it->skill_data.skill_id);

                if (!should_remove && prev_it != all_rotation_steps.begin())
                {
                    auto prev_it2 = std::prev(prev_it);
                    should_remove = (prev_it2->skill_data.skill_id == it->skill_data.skill_id);
                }
            }
        }

        if (should_remove)
            it = all_rotation_steps.erase(it);
        else
            ++it;
    }
}

SkillDataMap get_skill_data_map(const nlohmann::json &j)
{
    auto skill_data_map = SkillDataMap{};

    for (const auto &[skill_id_str, skill_obj] : j.items())
    {
        auto skill_id_int = std::stoi(skill_id_str);
        auto skill_id = static_cast<SkillID>(skill_id_int);
        auto skill_data = SkillData{};
        skill_data.skill_id = skill_id;

        if (skill_obj.contains("name") && skill_obj["name"].is_string())
            skill_data.name = skill_obj["name"].get<std::string>();

        if (skill_obj.contains("recharge") && skill_obj["recharge"].is_number())
        {
            skill_data.recharge_time = skill_obj["recharge"].get<int>();
            skill_data.recharge_time_with_alacrity = skill_data.recharge_time * 0.8f;
        }
        else
        {
            skill_data.recharge_time = -1;
            skill_data.recharge_time_with_alacrity = -1.0f;
        }

        const auto cast_time_it = SkillRuleData::skill_cast_time_map.find(skill_data.skill_id);
        if (cast_time_it != SkillRuleData::skill_cast_time_map.end())
        {
            skill_data.cast_time = cast_time_it->second;
            skill_data.cast_time_with_quickness = skill_data.cast_time * 0.8f;
        }
        else if (skill_obj.contains("cast_time") && skill_obj["cast_time"].is_number())
        {
            skill_data.cast_time = skill_obj["cast_time"].get<int>();
            skill_data.cast_time_with_quickness = skill_data.cast_time * 0.8f;
        }
        else
        {
            skill_data.cast_time = -1;
            skill_data.cast_time_with_quickness = -1.0f;
        }

        if (skill_obj.contains("is_auto_attack") && skill_obj["is_auto_attack"].is_boolean())
            skill_data.is_auto_attack = skill_obj["is_auto_attack"].get<bool>();
        else
            skill_data.is_auto_attack = false;

        if (skill_obj.contains("is_weapon_skill") && skill_obj["is_weapon_skill"].is_boolean())
            skill_data.is_weapon_skill = skill_obj["is_weapon_skill"].get<bool>();
        else
            skill_data.is_weapon_skill = false;

        if (skill_obj.contains("is_utility_skill") && skill_obj["is_utility_skill"].is_boolean())
            skill_data.is_utility_skill = skill_obj["is_utility_skill"].get<bool>();
        else
            skill_data.is_utility_skill = false;

        if (skill_obj.contains("is_elite_skill") && skill_obj["is_elite_skill"].is_boolean())
            skill_data.is_elite_skill = skill_obj["is_elite_skill"].get<bool>();
        else
            skill_data.is_elite_skill = false;

        if (skill_obj.contains("is_heal_skill") && skill_obj["is_heal_skill"].is_boolean())
            skill_data.is_heal_skill = skill_obj["is_heal_skill"].get<bool>();
        else
            skill_data.is_heal_skill = false;

        if (skill_obj.contains("is_profession_skill") && skill_obj["is_profession_skill"].is_boolean())
            skill_data.is_profession_skill = skill_obj["is_profession_skill"].get<bool>();
        else
            skill_data.is_profession_skill = false;

        if (skill_obj.contains("skill_type") && skill_obj["skill_type"].is_string())
        {
            const auto _type_str = skill_obj["skill_type"].get<std::string>();
            skill_data.skill_type = static_cast<SkillSlot>(std::stoi(_type_str));
        }
        else
            skill_data.skill_type = SkillSlot::NONE;
        if (skill_obj.contains("weapon_type") && skill_obj["weapon_type"].is_number_integer())
        {
            const auto val = skill_obj["weapon_type"].get<int>();
            skill_data.weapon_type = static_cast<WeaponType>(val);
        }
        else
            skill_data.weapon_type = WeaponType::NONE;

        skill_data.icon_id = 0; // Default value
        if (skill_obj.contains("icon") && skill_obj["icon"].is_string())
        {
            auto icon_url = skill_obj["icon"].get<std::string>();
            auto last_slash = icon_url.find_last_of('/');
            if (last_slash != std::string::npos)
            {
                auto filename = icon_url.substr(last_slash + 1);
                auto dot_pos = filename.find_last_of('.');
                if (dot_pos != std::string::npos)
                {
                    auto icon_id_str = filename.substr(0, dot_pos);
                    try
                    {
                        skill_data.icon_id = std::stoi(icon_id_str);
                    }
                    catch (const std::exception &)
                    {
                        skill_data.icon_id = 0;
                    }
                }
                else
                {
                    skill_data.icon_id = (int)SkillID::UNKNOWN_SKILL;

                    const auto msg = "Failed to parse icon ID from URL: " + icon_url +
                                     " for skill ID: " + std::to_string(skill_id_int) +
                                     " and Skill Name: " + skill_data.name;
                    (void)Globals::APIDefs->Log(LOGL_WARNING, "GW2RotaHelper", msg.c_str());
                }
            }
            else
            {
                skill_data.icon_id = (int)SkillID::UNKNOWN_SKILL;

                const auto msg = "Failed to parse icon ID from URL: " + icon_url +
                                 " for skill ID: " + std::to_string(skill_id_int) +
                                 " and Skill Name: " + skill_data.name;
                (void)Globals::APIDefs->Log(LOGL_WARNING, "GW2RotaHelper", msg.c_str());
            }
        }

        if (skill_data.name == "")
        {
            if (!skill_data.is_weapon_skill && !skill_data.is_auto_attack && !skill_data.is_utility_skill &&
                !skill_data.is_elite_skill && !skill_data.is_heal_skill && !skill_data.is_profession_skill)
            {
                skill_data.name = "Weapon Swap";
                skill_data.icon_id = (int)SkillID::WEAPON_SWAP;
            }
            else
            {
                skill_data.name = "Unknown";
                skill_data.icon_id = (int)SkillID::UNKNOWN_SKILL;

                const auto msg = "Failed to parse skill name: " + skill_data.name;
                (void)Globals::APIDefs->Log(LOGL_WARNING, "GW2RotaHelper", msg.c_str());
            }
        }

        skill_data_map[skill_id] = skill_data;
    }

    return skill_data_map;
}

SkillKeyMapping get_skill_key_mapping(const nlohmann::json &j)
{
    auto skill_key_mapping = SkillKeyMapping{};

    if (!j.contains("SkillKeyMapping"))
        return skill_key_mapping;

    const auto &build_meta = j["SkillKeyMapping"];

    if (build_meta.contains("slot_7") && build_meta["slot_7"].is_number_integer())
        skill_key_mapping.skill_7 = build_meta["slot_7"].get<int>();

    if (build_meta.contains("slot_8") && build_meta["slot_8"].is_number_integer())
        skill_key_mapping.skill_8 = build_meta["slot_8"].get<int>();

    if (build_meta.contains("slot_9") && build_meta["slot_9"].is_number_integer())
        skill_key_mapping.skill_9 = build_meta["slot_9"].get<int>();

    return skill_key_mapping;
}

MetaData get_metadata(const nlohmann::json &j)
{
    auto metadata = MetaData{};

    if (!j.contains("buildMetadata"))
        return metadata;

    const auto &build_meta = j["buildMetadata"];

    if (build_meta.contains("name") && build_meta["name"].is_string())
        metadata.name = build_meta["name"].get<std::string>();

    if (build_meta.contains("url") && build_meta["url"].is_string())
        metadata.url = build_meta["url"].get<std::string>();

    if (build_meta.contains("benchmark_type") && build_meta["benchmark_type"].is_string())
        metadata.benchmark_type = build_meta["benchmark_type"].get<std::string>();

    if (build_meta.contains("profession") && build_meta["profession"].is_string())
        metadata.profession = build_meta["profession"].get<std::string>();

    if (build_meta.contains("elite_spec") && build_meta["elite_spec"].is_string())
        metadata.elite_spec = build_meta["elite_spec"].get<std::string>();

    if (build_meta.contains("build_type") && build_meta["build_type"].is_string())
        metadata.build_type = build_meta["build_type"].get<std::string>();

    if (build_meta.contains("url_name") && build_meta["url_name"].is_string())
        metadata.url_name = build_meta["url_name"].get<std::string>();

    if (build_meta.contains("dps_report_url") && build_meta["dps_report_url"].is_string())
        metadata.dps_report_url = build_meta["dps_report_url"].get<std::string>();

    if (build_meta.contains("html_file_path") && build_meta["html_file_path"].is_string())
        metadata.html_file_path = build_meta["html_file_path"].get<std::string>();

    if (build_meta.contains("overall_dps") && build_meta["overall_dps"].is_number_float())
        metadata.overall_dps = build_meta["overall_dps"].get<double>();

    metadata.elite_spec_id = string_to_elite_spec(metadata.elite_spec);
    metadata.profession_id = string_to_profession(metadata.profession);

    return metadata;
}

std::tuple<LogSkillInfoMap, RotationSteps, RotationSteps, MetaData, SkillKeyMapping> get_dpsreport_data(
    const std::filesystem::path &json_path,
    const SkillDataMap &skill_data_map)
{
    auto json_rotation_log = nlohmann::json{};
    auto is_load_success = load_rotaion_json(json_path, json_rotation_log);
    if (!is_load_success)
        return std::make_tuple(LogSkillInfoMap{}, RotationSteps{}, RotationSteps{}, MetaData{}, SkillKeyMapping{});

    const auto rotation_data = json_rotation_log["rotation"];
    const auto skill_data = json_rotation_log["skillMap"];

    auto kv_rotation = IntNode{};
    collect_json(rotation_data, kv_rotation);
    auto kv_skill = IntNode{};
    collect_json(skill_data, kv_skill, true);

    auto log_skill_info_map = LogSkillInfoMap{};
    get_skill_info(kv_skill, log_skill_info_map);
    auto rotation_steps = RotationSteps{};
    get_rotation_info(kv_rotation,
                      log_skill_info_map,
                      rotation_steps,
                      skill_data_map,
                      Settings::StrictModeForSkillDetection,
                      Settings::ShowWeaponSwap,
                      Settings::EasySkillMode);
    auto rotation_steps_w_swap = RotationSteps{};
    get_rotation_info(kv_rotation,
                      log_skill_info_map,
                      rotation_steps_w_swap,
                      skill_data_map,
                      Settings::StrictModeForSkillDetection,
                      true,
                      Settings::EasySkillMode);

    auto metadata = get_metadata(json_rotation_log);
    auto skill_key_mapping = get_skill_key_mapping(json_rotation_log);

    return std::make_tuple(log_skill_info_map, rotation_steps, rotation_steps_w_swap, metadata, skill_key_mapping);
}

bool DownloadFileFromURL(const std::string &url, const std::filesystem::path &out_path)
{
    const auto hInternet = InternetOpenA("GW2RotaHelper", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!hInternet)
    {
        (void)Globals::APIDefs->Log(LOGL_CRITICAL, "GW2RotaHelper", "Failed to open internet connection");
        return false;
    }

    const auto hFile = InternetOpenUrlA(hInternet, url.c_str(), NULL, 0, INTERNET_FLAG_RELOAD, 0);
    if (!hFile)
    {
        (void)Globals::APIDefs->Log(LOGL_CRITICAL, "GW2RotaHelper", "Failed to open URL");
        InternetCloseHandle(hInternet);
        return false;
    }

    auto outFile = std::ofstream{out_path, std::ios::binary};
    if (!outFile.is_open())
    {
        (void)Globals::APIDefs->Log(LOGL_CRITICAL, "GW2RotaHelper", "Failed to create output file");
        InternetCloseHandle(hFile);
        InternetCloseHandle(hInternet);
        return false;
    }

    auto buffer = std::array<char, 4096>{};
    auto bytesRead = DWORD{0};
    auto success = true;

    do
    {
        if (InternetReadFile(hFile, buffer.data(), buffer.size(), &bytesRead))
        {
            if (bytesRead > 0)
            {
                outFile.write(buffer.data(), bytesRead);
                if (outFile.fail())
                {
                    (void)Globals::APIDefs->Log(LOGL_CRITICAL, "GW2RotaHelper", "Failed to write to file");
                    success = false;
                    break;
                }
            }
        }
        else
        {
            (void)Globals::APIDefs->Log(LOGL_CRITICAL, "GW2RotaHelper", "Failed to read from URL");
            success = false;
            break;
        }
    } while (bytesRead > 0);

    // Cleanup
    outFile.close();
    InternetCloseHandle(hFile);
    InternetCloseHandle(hInternet);

    if (success)
    {
        (void)Globals::APIDefs->Log(LOGL_DEBUG, "GW2RotaHelper", "Successfully downloaded file");
    }
    else
    {
        std::filesystem::remove(out_path);
    }

    return success;
}

std::list<std::future<void>> StartDownloadAllSkillIcons(const LogSkillInfoMap &log_skill_info_map,
                                                        const std::filesystem::path &img_folder)
{
    std::filesystem::create_directories(img_folder);
    auto futures = std::list<std::future<void>>{};

    for (const auto &[icon_id, info] : log_skill_info_map)
    {
        if (info.icon_url.empty() || info.icon_url.find("local://") == 0)
            continue;

        auto ext{std::string{".png"}};
        const auto dot_pos{info.icon_url.find_last_of('.')};
        if (dot_pos != std::string::npos && dot_pos + 1 < info.icon_url.size())
        {
            ext = info.icon_url.substr(dot_pos);
        }
        const auto out_path{img_folder / (std::to_string(icon_id) + ext)};
        if (std::filesystem::exists(out_path))
            continue;

        (void)Globals::APIDefs->Log(LOGL_DEBUG, "GW2RotaHelper", "Downloading file");
        futures.emplace_back(
            std::async(std::launch::async, [url = info.icon_url, out_path]() { DownloadFileFromURL(url, out_path); }));
    }

    return futures;
}
} // namespace

bool is_skill_in_set(const std::string &skill_name, const std::set<std::string> &set, const bool exact_match)
{
    for (const auto &filter_string : set)
    {
        if (exact_match)
        {
            if (skill_name == filter_string)
                return true;
        }
        else
        {
            if (skill_name.find(filter_string) != std::string::npos)
                return true;
        }
    }

    return false;
}

bool is_skill_in_set(std::string_view skill_name, const std::set<std::string_view> &set, const bool exact_match)
{
    for (const auto &filter_string : set)
    {
        if (exact_match)
        {
            if (skill_name == filter_string)
                return true;
        }
        else
        {
            if (skill_name.find(filter_string) != std::string::npos)
                return true;
        }
    }

    return false;
}

bool is_skill_in_set(SkillID skill_id, const std::set<SkillID> &set)
{
    if (set.find(skill_id) != set.end())
        return true;

    return false;
}

SkillState get_skill_state(const RotationLogType &rotation_run,
                           const std::vector<EvCombatDataPersistent> &played_rotation,
                           const size_t window_idx,
                           const size_t current_idx,
                           const bool is_auto_attack)
{
    const auto is_current = (window_idx == static_cast<int32_t>(current_idx));
    const auto is_last = (window_idx == Globals::RotationRun.all_rotation_steps.size() - 1);

    return SkillState{
        .is_current = is_current,
        .is_last = is_last,
        .is_auto_attack = is_auto_attack,
    };
}

void RotationLogType::calculate_auto_attack_indices()
{
    const auto &all_steps = all_rotation_steps;
    auto &indices = auto_attack_indices;

    indices.clear();
    indices.resize(all_steps.size(), 0);

    for (size_t i = 0; i < all_steps.size(); ++i)
    {
        if (!all_steps[i].skill_data.is_auto_attack)
            continue;

        auto sequence_start = i;
        while (sequence_start > 0 && all_steps[sequence_start - 1].skill_data.is_auto_attack)
            sequence_start--;

        auto sequence_end = i;
        while (sequence_end < all_steps.size() - 1 && all_steps[sequence_end + 1].skill_data.is_auto_attack)
            sequence_end++;

        auto sequence_length = sequence_end - sequence_start + 1;
        if (sequence_length > 1)
        {
            for (size_t j = sequence_start; j <= sequence_end; ++j)
            {
                indices[j] = static_cast<int>(j - sequence_start + 1);
            }
        }
    }
}

void RotationLogType::load_data(const std::filesystem::path &json_path, const std::filesystem::path &img_path)
{
    skill_data_map.clear();
    log_skill_info_map.clear();
    all_rotation_steps.clear();
    auto_attack_indices.clear();

    auto jsons_skill_data = nlohmann::json{};
    const auto load_success = load_skill_data_map(json_path, jsons_skill_data);
    if (!load_success)
        return;
    skill_data_map = get_skill_data_map(jsons_skill_data);

    const auto [_skill_info_map,
                _bench_all_rotation_steps,
                _bench_all_rotation_steps_w_swap,
                _meta_data,
                _skill_key_mapping] = get_dpsreport_data(json_path, skill_data_map);

    log_skill_info_map = _skill_info_map;
    all_rotation_steps = _bench_all_rotation_steps;
    all_rotation_steps_w_swap = _bench_all_rotation_steps_w_swap;
    meta_data = _meta_data;
    skill_key_mapping = _skill_key_mapping;

    restart_rotation();

    if (!all_rotation_steps.empty())
        calculate_auto_attack_indices();

    futures = StartDownloadAllSkillIcons(log_skill_info_map, img_path);
    (void)Globals::APIDefs->Log(LOGL_DEBUG, "GW2RotaHelper", "Loaded Rotation");
}

void RotationLogType::pop_bench_rotation_queue()
{
    if (!missing_rotation_steps.empty())
        missing_rotation_steps.pop_front();
}

std::tuple<std::int32_t, std::int32_t, size_t> RotationLogType::get_current_rotation_indices() const
{
    constexpr static auto window_size = 10;
    constexpr static auto window_size_left = 2;
    constexpr static auto window_size_right = window_size - window_size_left - 1;

    if (missing_rotation_steps.empty())
        return {-1, -1, static_cast<size_t>(-1)};

    const auto num_skills_left = static_cast<int64_t>(missing_rotation_steps.size());
    const auto num_total_skills = static_cast<int64_t>(all_rotation_steps.size());

    auto current_idx = static_cast<size_t>(num_total_skills - num_skills_left);
    const auto start = current_idx - 2 >= 0 ? static_cast<int32_t>(current_idx - window_size_left) : 0;
    const auto end =
        current_idx + window_size < num_total_skills
            ? start > 0 ? static_cast<int32_t>(current_idx + window_size_right) : static_cast<int32_t>(window_size - 1)
            : static_cast<std::int32_t>(num_total_skills - 1);

    return {start, end, current_idx};
}

RotationStep RotationLogType::get_rotation_skill(const size_t idx) const
{
    if (idx < all_rotation_steps.size())
        return all_rotation_steps.at(idx);

    return RotationStep{};
}

void RotationLogType::restart_rotation()
{
    missing_rotation_steps = std::list<RotationStep>(all_rotation_steps.begin(), all_rotation_steps.end());
}

bool RotationLogType::is_current_run_done() const
{
    return missing_rotation_steps.empty();
}

void RotationLogType::reset_rotation()
{
    log_skill_info_map.clear();
    all_rotation_steps.clear();
    missing_rotation_steps.clear();
    skill_data_map.clear();
    meta_data = MetaData{};
    auto_attack_indices.clear();
    (void)Globals::APIDefs->Log(LOGL_DEBUG, "GW2RotaHelper", "Resetted Rotation");
}

std::string RotationLogType::get_keybind_str(const RotationStep &rotation_step,
                                             const std::map<std::string, KeybindInfo> &keybinds)
{
    const auto &skill_key_mapping = Globals::RotationRun.skill_key_mapping;
    const auto &log_skill_info_map = Globals::RotationRun.log_skill_info_map;
    const auto skill_data = rotation_step.skill_data;

    std::string keybind_str;

    const auto skill_name_for_slot7 =
        skill_key_mapping.skill_7 != -1 &&
                log_skill_info_map.find(skill_key_mapping.skill_7) != log_skill_info_map.end()
            ? log_skill_info_map.find(skill_key_mapping.skill_7)->second.name
            : "";
    const auto skill_name_for_slot8 =
        skill_key_mapping.skill_8 != -1 &&
                log_skill_info_map.find(skill_key_mapping.skill_8) != log_skill_info_map.end()
            ? log_skill_info_map.find(skill_key_mapping.skill_8)->second.name
            : "";
    const auto skill_name_for_slot9 =
        skill_key_mapping.skill_9 != -1 &&
                log_skill_info_map.find(skill_key_mapping.skill_9) != log_skill_info_map.end()
            ? log_skill_info_map.find(skill_key_mapping.skill_9)->second.name
            : "";

    if (rotation_step.skill_data.name == skill_name_for_slot7)
    {
        keybind_str = "7";
    }
    else if (rotation_step.skill_data.name == skill_name_for_slot8)
    {
        keybind_str = "8";
    }
    else if (rotation_step.skill_data.name == skill_name_for_slot9)
    {
        keybind_str = "9";
    }
    else
    {
        if (Settings::XmlSettingsPath.empty())
        {
            keybind_str = default_skillslot_to_string(skill_data.skill_type);
        }
        else
        {
            const auto [keybind, modifier] = get_keybind_for_skill_type(skill_data.skill_type, keybinds);
            if (keybind == Keys::NONE)
            {
                keybind_str = default_skillslot_to_string(skill_data.skill_type);
            }
            else
            {
                keybind_str = custom_keys_to_string(keybind);
                if (modifier != Modifiers::NONE)
                {
                    keybind_str = "(" + modifiers_to_string(modifier) + " + " + keybind_str + ")";
                }
            }
        }
    }

    return keybind_str;
}

void RotationLogType::get_rotation_icons()
{
    rotation_icon_lines.clear();
    auto rotation_line = std::vector<std::pair<ID3D11ShaderResourceView *, std::string>>{};

    const auto &rotation = Globals::RotationRun.all_rotation_steps_w_swap;

    for (const auto &rotation_step : rotation)
    {
        const auto skill_data =
            SkillRuleData::GetDataByID(rotation_step.skill_data.skill_id, Globals::RotationRun.skill_data_map);

        const auto icon_it = Globals::TextureMap.find(skill_data.icon_id);

        if (is_skill_in_set(skill_data.name, SkillRuleData::skill_rules.skills_match_weapon_swap_like))
        {
            rotation_icon_lines.push_back(rotation_line);
            rotation_line = {};
            continue;
        }

        if (icon_it != Globals::TextureMap.end())
        {
            if (!icon_it->second)
                continue;
            rotation_line.push_back(std::make_pair(icon_it->second, skill_data.name));
        }
        else
        {
            continue;
        }
    }

    if (!rotation_line.empty())
    {
        rotation_icon_lines.push_back(rotation_line);
    }
}

void RotationLogType::get_rotation_text(const std::map<std::string, KeybindInfo> &keybinds)
{
    std::string line;
    rotation_text.clear();

    bool first_in_line = true;

    const auto &rotation = Globals::RotationRun.all_rotation_steps_w_swap;

    auto prev_was_aa = false;
    for (const auto &rotation_step : rotation)
    {
        const auto skill_data = rotation_step.skill_data;
        const auto weapon_type = skill_data.weapon_type;
        const auto weapon_type_str = weapon_type_to_string(weapon_type);
        const auto keybind_str = get_keybind_str(rotation_step, keybinds);

        if (is_skill_in_set(skill_data.name, SkillRuleData::skill_rules.skills_match_weapon_swap_like))
        {
            if (line == "")
                continue;

            if (line.find(": ") == std::string::npos)
            {
                // XXX: Hacky Solution
                auto kit_name = std::string{"Utility"};
                if (Globals::Identity.Profession == Mumble::EProfession::Engineer)
                {
                    if (skill_data.name.contains("Kit") || skill_data.name.contains("Gun") ||
                        skill_data.name.contains("Flamethrower"))
                        kit_name = "Kit";
                    else
                        kit_name = "Forge";
                }
                else if (Globals::Identity.Profession == Mumble::EProfession::Necromancer)
                    kit_name = "Shroud";
                else if (Globals::Identity.Profession == Mumble::EProfession::Revenant)
                    kit_name = "Stance";
                else if (Globals::Identity.Profession == Mumble::EProfession::Elementalist)
                    kit_name = "Attunement";
                else if (static_cast<EliteSpecID>(Globals::Identity.Specialization) == EliteSpecID::Druid)
                    kit_name = "Avatar";
                else if (static_cast<EliteSpecID>(Globals::Identity.Specialization) == EliteSpecID::Galeshot)
                    kit_name = "Cyclone";
                else if (static_cast<EliteSpecID>(Globals::Identity.Specialization) == EliteSpecID::Untamed)
                    kit_name = "Unleash";
                else if (static_cast<EliteSpecID>(Globals::Identity.Specialization) == EliteSpecID::Luminary)
                    kit_name = "Forge"; // TODO: Add in skills_match_weapon_swap_like
                else if (static_cast<EliteSpecID>(Globals::Identity.Specialization) == EliteSpecID::Firebrand)
                    kit_name = "Tome"; // TODO: Add in skills_match_weapon_swap_like
                else if (static_cast<EliteSpecID>(Globals::Identity.Specialization) == EliteSpecID::Bladesworn)
                    kit_name = "Gunsaber";

                line = kit_name + ": " + line;
            }

            line += "\n";
            rotation_text.push_back(line);
            line.clear();
            first_in_line = true;
            prev_was_aa = false;
        }
        else
        {
            if (line == "" && weapon_type_str != "None")
            {
                line = weapon_type_str + ": ";
            }

            if (!first_in_line)
            {
                line += " - ";
            }

            auto curr_is_aa = skill_data.skill_type == SkillSlot::WEAPON_1;

            if (keybind_str != "")
            {
                if (prev_was_aa && curr_is_aa)
                {
                    const auto search_str = std::string{"xAA"};
                    const auto slcie_size = search_str.size() + 4;
                    auto keep_slice = line.size() >= slcie_size ? line.substr(0, line.size() - slcie_size) : line;
                    auto prev_counter =
                        line.size() >= slcie_size ? line.substr(line.size() - slcie_size, slcie_size) : line;

                    if (prev_counter.find(search_str) != std::string::npos)
                    {
                        auto found_idx = prev_counter.find("x");
                        auto count_str = prev_counter.substr(0, found_idx);
                        int count = 0;
                        try
                        {
                            count = std::stoi(count_str);
                        }
                        catch (...)
                        {
                            count = 1;
                        }
                        count++;
                        line = keep_slice + std::to_string(count) + "xAA";
                    }
                }
                else if (curr_is_aa)
                {
                    line += "1xAA";
                }
                else
                {
                    line += keybind_str;
                }
            }

            prev_was_aa = curr_is_aa;
            first_in_line = false;
        }
    }

    if (line != "")
    {
        rotation_text.push_back(line);
    }
}
