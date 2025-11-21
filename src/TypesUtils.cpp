#pragma once

#include <algorithm>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "FileUtils.h"
#include "Types.h"

ProfessionID string_to_profession(const std::string &profession_name)
{
    auto lower_name = profession_name;
    std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);

    if (lower_name == "guardian")
        return ProfessionID::GUARDIAN;
    if (lower_name == "warrior")
        return ProfessionID::WARRIOR;
    if (lower_name == "engineer")
        return ProfessionID::ENGINEER;
    if (lower_name == "ranger")
        return ProfessionID::RANGER;
    if (lower_name == "thief")
        return ProfessionID::THIEF;
    if (lower_name == "elementalist")
        return ProfessionID::ELEMENTALIST;
    if (lower_name == "mesmer")
        return ProfessionID::MESMER;
    if (lower_name == "necromancer")
        return ProfessionID::NECROMANCER;
    if (lower_name == "revenant")
        return ProfessionID::REVENANT;

    return ProfessionID::UNKNOWN;
}

EliteSpecID string_to_elite_spec(const std::string &spec_name)
{
    auto lower_name = spec_name;
    std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);

    // Elementalist
    if (lower_name == "catalyst")
        return EliteSpecID::Catalyst;
    if (lower_name == "evoker")
        return EliteSpecID::Evoker;
    if (lower_name == "tempest")
        return EliteSpecID::Tempest;
    if (lower_name == "weaver")
        return EliteSpecID::Weaver;

    // Engineer
    if (lower_name == "amalgam")
        return EliteSpecID::Amalgam;
    if (lower_name == "holosmith")
        return EliteSpecID::Holosmith;
    if (lower_name == "mechanist")
        return EliteSpecID::Mechanist;
    if (lower_name == "scrapper")
        return EliteSpecID::Scrapper;

    // Guardian
    if (lower_name == "dragonhunter")
        return EliteSpecID::Dragonhunter;
    if (lower_name == "firebrand")
        return EliteSpecID::Firebrand;
    if (lower_name == "luminary")
        return EliteSpecID::Luminary;
    if (lower_name == "willbender")
        return EliteSpecID::Willbender;

    // Mesmer
    if (lower_name == "chronomancer")
        return EliteSpecID::Chronomancer;
    if (lower_name == "mirage")
        return EliteSpecID::Mirage;
    if (lower_name == "troubadour")
        return EliteSpecID::Troubadour;
    if (lower_name == "virtuoso")
        return EliteSpecID::Virtuoso;

    // Necromancer
    if (lower_name == "harbinger")
        return EliteSpecID::Harbinger;
    if (lower_name == "reaper")
        return EliteSpecID::Reaper;
    if (lower_name == "ritualist")
        return EliteSpecID::Ritualist;
    if (lower_name == "scourge")
        return EliteSpecID::Scourge;

    // Ranger
    if (lower_name == "druid")
        return EliteSpecID::Druid;
    if (lower_name == "galeshot")
        return EliteSpecID::Galeshot;
    if (lower_name == "soulbeast")
        return EliteSpecID::Soulbeast;
    if (lower_name == "untamed")
        return EliteSpecID::Untamed;

    // Revenant
    if (lower_name == "conduit")
        return EliteSpecID::Conduit;
    if (lower_name == "herald")
        return EliteSpecID::Herald;
    if (lower_name == "renegade")
        return EliteSpecID::Renegade;
    if (lower_name == "vindicator")
        return EliteSpecID::Vindicator;

    // Thief
    if (lower_name == "antiquary")
        return EliteSpecID::Antiquary;
    if (lower_name == "daredevil")
        return EliteSpecID::Daredevil;
    if (lower_name == "deadeye")
        return EliteSpecID::Deadeye;
    if (lower_name == "specter" || lower_name == "spectre")
        return EliteSpecID::Specter;

    // Warrior
    if (lower_name == "berserker")
        return EliteSpecID::Berserker;
    if (lower_name == "bladesworn")
        return EliteSpecID::Bladesworn;
    if (lower_name == "paragon")
        return EliteSpecID::Paragon;
    if (lower_name == "spellbreaker")
        return EliteSpecID::Spellbreaker;

    return EliteSpecID::Unknown;
}

std::string profession_to_string(ProfessionID profession_id)
{
    switch (profession_id)
    {
    case ProfessionID::GUARDIAN:
        return "Guardian";
    case ProfessionID::WARRIOR:
        return "Warrior";
    case ProfessionID::ENGINEER:
        return "Engineer";
    case ProfessionID::RANGER:
        return "Ranger";
    case ProfessionID::THIEF:
        return "Thief";
    case ProfessionID::ELEMENTALIST:
        return "Elementalist";
    case ProfessionID::MESMER:
        return "Mesmer";
    case ProfessionID::NECROMANCER:
        return "Necromancer";
    case ProfessionID::REVENANT:
        return "Revenant";
    case ProfessionID::UNKNOWN:
    default:
        return "Unknown";
    }
}

std::string elite_spec_to_string(EliteSpecID elite_spec_id)
{
    switch (elite_spec_id)
    {
    // Elementalist
    case EliteSpecID::Catalyst:
        return "Catalyst";
    case EliteSpecID::Evoker:
        return "Evoker";
    case EliteSpecID::Tempest:
        return "Tempest";
    case EliteSpecID::Weaver:
        return "Weaver";

    // Engineer
    case EliteSpecID::Amalgam:
        return "Amalgam";
    case EliteSpecID::Holosmith:
        return "Holosmith";
    case EliteSpecID::Mechanist:
        return "Mechanist";
    case EliteSpecID::Scrapper:
        return "Scrapper";

    // Guardian
    case EliteSpecID::Dragonhunter:
        return "Dragonhunter";
    case EliteSpecID::Firebrand:
        return "Firebrand";
    case EliteSpecID::Luminary:
        return "Luminary";
    case EliteSpecID::Willbender:
        return "Willbender";

    // Mesmer
    case EliteSpecID::Chronomancer:
        return "Chronomancer";
    case EliteSpecID::Mirage:
        return "Mirage";
    case EliteSpecID::Troubadour:
        return "Troubadour";
    case EliteSpecID::Virtuoso:
        return "Virtuoso";

    // Necromancer
    case EliteSpecID::Harbinger:
        return "Harbinger";
    case EliteSpecID::Reaper:
        return "Reaper";
    case EliteSpecID::Ritualist:
        return "Ritualist";
    case EliteSpecID::Scourge:
        return "Scourge";

    // Ranger
    case EliteSpecID::Druid:
        return "Druid";
    case EliteSpecID::Galeshot:
        return "Galeshot";
    case EliteSpecID::Soulbeast:
        return "Soulbeast";
    case EliteSpecID::Untamed:
        return "Untamed";

    // Revenant
    case EliteSpecID::Conduit:
        return "Conduit";
    case EliteSpecID::Herald:
        return "Herald";
    case EliteSpecID::Renegade:
        return "Renegade";
    case EliteSpecID::Vindicator:
        return "Vindicator";

    // Thief
    case EliteSpecID::Antiquary:
        return "Antiquary";
    case EliteSpecID::Daredevil:
        return "Daredevil";
    case EliteSpecID::Deadeye:
        return "Deadeye";
    case EliteSpecID::Specter:
        return "Specter";

    // Warrior
    case EliteSpecID::Berserker:
        return "Berserker";
    case EliteSpecID::Bladesworn:
        return "Bladesworn";
    case EliteSpecID::Paragon:
        return "Paragon";
    case EliteSpecID::Spellbreaker:
        return "Spellbreaker";

    // Default/Unknown
    case EliteSpecID::Unknown:
    default:
        return "Unknown";
    }
}

std::vector<std::string> get_elite_specs_for_profession(ProfessionID profession)
{
    std::vector<std::string> elite_specs;

    switch (profession)
    {
    case ProfessionID::GUARDIAN:
        elite_specs = {"dragonhunter", "firebrand", "willbender", "luminary"};
        break;
    case ProfessionID::WARRIOR:
        elite_specs = {"berserker", "spellbreaker", "bladesworn", "paragon"};
        break;
    case ProfessionID::ENGINEER:
        elite_specs = {"scrapper", "holosmith", "mechanist", "amalgam"};
        break;
    case ProfessionID::RANGER:
        elite_specs = {"druid", "soulbeast", "untamed", "galeshot"};
        break;
    case ProfessionID::THIEF:
        elite_specs = {"daredevil", "deadeye", "specter", "antiquary"};
        break;
    case ProfessionID::ELEMENTALIST:
        elite_specs = {"tempest", "weaver", "catalyst", "evoker"};
        break;
    case ProfessionID::MESMER:
        elite_specs = {"chronomancer", "mirage", "virtuoso", "troubadour"};
        break;
    case ProfessionID::NECROMANCER:
        elite_specs = {"reaper", "scourge", "harbinger", "ritualist"};
        break;
    case ProfessionID::REVENANT:
        elite_specs = {"herald", "renegade", "vindicator", "conduit"};
        break;
    default:
        break;
    }

    return elite_specs;
}

std::string skillslot_to_string(SkillSlot skill_type)
{
    switch (skill_type)
    {
    case SkillSlot::WEAPON_1:
        return "1";
    case SkillSlot::WEAPON_2:
        return "2";
    case SkillSlot::WEAPON_3:
        return "3";
    case SkillSlot::WEAPON_4:
        return "4";
    case SkillSlot::WEAPON_5:
        return "5";
    case SkillSlot::HEAL:
        return "6";
    case SkillSlot::UTILITY_1:
        return "Util"; // XXX: n/a about skill slot
    case SkillSlot::UTILITY_2:
        return "Util"; // XXX n/a about skill slot
    case SkillSlot::UTILITY_3:
        return "Util"; // XXX n/a about skill slot
    case SkillSlot::ELITE:
        return "0";
    case SkillSlot::PROFESSION_1:
        return "F1";
    case SkillSlot::PROFESSION_2:
        return "F2";
    case SkillSlot::PROFESSION_3:
        return "F3";
    case SkillSlot::PROFESSION_4:
        return "F4";
    case SkillSlot::PROFESSION_5:
        return "F5";
    case SkillSlot::PROFESSION_6:
        return "F6";
    case SkillSlot::PROFESSION_7:
        return "F7";
    default:
        return "";
    }
}

SkillSlot str_to_skillslot(const std::string keybind_str)
{
    if (keybind_str == "Weapon_1")
        return SkillSlot::WEAPON_1;
    if (keybind_str == "Weapon_2")
        return SkillSlot::WEAPON_2;
    if (keybind_str == "Weapon_3")
        return SkillSlot::WEAPON_3;
    if (keybind_str == "Weapon_4")
        return SkillSlot::WEAPON_4;
    if (keybind_str == "Weapon_5")
        return SkillSlot::WEAPON_5;
    if (keybind_str == "Heal")
        return SkillSlot::HEAL;
    if (keybind_str == "Utility_1")
        return SkillSlot::UTILITY_1;
    if (keybind_str == "Utility_2")
        return SkillSlot::UTILITY_2;
    if (keybind_str == "Utility_3")
        return SkillSlot::UTILITY_3;
    if (keybind_str == "Elite")
        return SkillSlot::ELITE;
    if (keybind_str == "Profession_1")
        return SkillSlot::PROFESSION_1;
    if (keybind_str == "Profession_2")
        return SkillSlot::PROFESSION_2;
    if (keybind_str == "Profession_3")
        return SkillSlot::PROFESSION_3;
    if (keybind_str == "Profession_4")
        return SkillSlot::PROFESSION_4;
    if (keybind_str == "Profession_5")
        return SkillSlot::PROFESSION_5;
    if (keybind_str == "Profession_6")
        return SkillSlot::PROFESSION_6;
    if (keybind_str == "Profession_7")
        return SkillSlot::PROFESSION_7;
    return SkillSlot::NONE;
}

std::string custom_keys_to_string(Keys key)
{
    switch (key)
    {
    case Keys::NONE:
        return "None";
    case Keys::LEFT_CTRL:
        return "LCtrl";
    case Keys::LEFT_SHIFT:
        return "LShift";
    case Keys::CAPS:
        return "Caps";
    case Keys::ZIRUMFLEX:
        return "^";
    case Keys::TAB:
        return "Tab";
    case Keys::F1:
        return "F1";
    case Keys::F2:
        return "F2";
    case Keys::F3:
        return "F3";
    case Keys::F4:
        return "F4";
    case Keys::F5:
        return "F5";
    case Keys::F6:
        return "F6";
    case Keys::F7:
        return "F7";
    case Keys::ONE:
        return "1";
    case Keys::TWO:
        return "2";
    case Keys::THREE:
        return "3";
    case Keys::FOUR:
        return "4";
    case Keys::FIVE:
        return "5";
    case Keys::SIX:
        return "6";
    case Keys::SEVEN:
        return "7";
    case Keys::EIGHT:
        return "8";
    case Keys::NINE:
        return "9";
    case Keys::ZERO:
        return "0";
    case Keys::A:
        return "A";
    case Keys::B:
        return "B";
    case Keys::C:
        return "C";
    case Keys::D:
        return "D";
    case Keys::E:
        return "E";
    case Keys::F:
        return "F";
    case Keys::G:
        return "G";
    case Keys::H:
        return "H";
    case Keys::I:
        return "I";
    case Keys::J:
        return "J";
    case Keys::K:
        return "K";
    case Keys::L:
        return "L";
    case Keys::M:
        return "M";
    case Keys::N:
        return "N";
    case Keys::O:
        return "O";
    case Keys::P:
        return "P";
    case Keys::Q:
        return "Q";
    case Keys::R:
        return "R";
    case Keys::S:
        return "S";
    case Keys::T:
        return "T";
    case Keys::U:
        return "U";
    case Keys::V:
        return "V";
    case Keys::W:
        return "W";
    case Keys::X:
        return "X";
    case Keys::Y:
        return "Y";
    case Keys::Z:
        return "Z";
    case Keys::LEFT_ALT:
        return "ALT";
    case Keys::LEFT_ARROW:
        return "Left";
    case Keys::RIGHT_ARROW:
        return "Right";
    case Keys::NUM_ADD:
        return "Num+";
    case Keys::NUM_1:
        return "Num1";
    case Keys::NUM_2:
        return "Num2";
    case Keys::NUM_3:
        return "Num3";
    case Keys::NUM_4:
        return "Num4";
    case Keys::NUM_5:
        return "Num5";
    case Keys::NUM_6:
        return "Num6";
    case Keys::NUM_7:
        return "Num7";
    case Keys::NUM_8:
        return "Num8";
    case Keys::NUM_9:
        return "Num9";
    case Keys::NUM_RET:
        return "NumEnter";
    default:
        return "Unknown";
    }
}

std::string modifiers_to_string(Modifiers modifier)
{
    switch (modifier)
    {
    case Modifiers::NONE:
        return "None";
    case Modifiers::SHIFT:
        return "Shift";
    case Modifiers::ALT:
        return "Alt";
    case Modifiers::CTRL:
        return "Ctrl";
    default:
        return "Unknown";
    }
}

std::pair<Keys, Modifiers> get_keybind_for_skill_type(SkillSlot skill_type,
                                                      const std::map<std::string, KeybindInfo> &keybinds)
{
    std::string action_name;

    switch (skill_type)
    {
    case SkillSlot::PROFESSION_1:
        action_name = "Profession Skill 1";
        break;
    case SkillSlot::PROFESSION_2:
        action_name = "Profession Skill 2";
        break;
    case SkillSlot::PROFESSION_3:
        action_name = "Profession Skill 3";
        break;
    case SkillSlot::PROFESSION_4:
        action_name = "Profession Skill 4";
        break;
    case SkillSlot::PROFESSION_5:
        action_name = "Profession Skill 5";
        break;
    case SkillSlot::PROFESSION_7:
        action_name = "Profession Skill 7";
        break;
    case SkillSlot::HEAL:
        action_name = "Healing Skill";
        break;
    case SkillSlot::UTILITY_1:
        action_name = "Utility Skill 1";
        break;
    case SkillSlot::UTILITY_2:
        action_name = "Utility Skill 2";
        break;
    case SkillSlot::UTILITY_3:
        action_name = "Utility Skill 3";
        break;
    case SkillSlot::ELITE:
        action_name = "Elite Skill";
        break;
    default:
        return std::make_pair(Keys::NONE, Modifiers::NONE);
    }

    auto it = keybinds.find(action_name);
    if (it != keybinds.end())
    {
        return std::make_pair(it->second.button, it->second.modifier);
    }

    return std::make_pair(Keys::NONE, Modifiers::NONE);
}

std::string download_state_to_string(DownloadState state)
{
    switch (state)
    {
    case DownloadState::NOT_STARTED:
        return "Not Started";
    case DownloadState::STARTED:
        return "In Progress";
    case DownloadState::FINISHED:
        return "Completed";
    case DownloadState::FAILED:
        return "Failed";
    case DownloadState::NO_UPDATE_NEEDED:
        return "No Update Needed";
    default:
        return "Unknown";
    }
}
