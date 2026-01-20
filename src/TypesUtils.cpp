#pragma once

#include <algorithm>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "FileUtils.h"
#include "Shared.h"
#include "SkillData.h"
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

std::string default_skillslot_to_string(SkillSlot skill_type)
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
        return "Util";
    case SkillSlot::UTILITY_2:
        return "Util";
    case SkillSlot::UTILITY_3:
        return "Util";
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

SkillSlot str_to_default_skillslot(const std::string keybind_str)
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
    case Keys::AE:
        return "Ae";
    case Keys::CAPS:
        return "Caps";
    case Keys::MINUS:
        return "-";
    case Keys::EQUAL:
        return "=";
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
    case Keys::SMALLER:
        return "<";
    default:
        return "Unknown";
    }
}

std::string modifiers_to_string(Modifiers modifier)
{
    switch (modifier)
    {
    case Modifiers::NONE: /* 0 */
        return "None";
    case Modifiers::SHIFT: /*1 */
        return "Shift";
    case Modifiers::ALT:  /* 4 */
    case Modifiers::RALT: /* 3 */
        return "Alt";
    case Modifiers::CTRL:  /* 2 */
    case Modifiers::RCTRL: /* 6 */
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

SkillID SafeConvertToSkillID(uint64_t skill_id_raw)
{
    auto skill_id = static_cast<SkillID>(skill_id_raw);

    const auto &skill_data_map = Globals::RotationRun.skill_data_map;
    if (skill_data_map.find(skill_id) == skill_data_map.end())
    {
        if (SkillRuleData::unk_skill_id_fix.find(skill_id_raw) != SkillRuleData::unk_skill_id_fix.end())
            return static_cast<SkillID>(SkillRuleData::unk_skill_id_fix.at(skill_id_raw));

        return SkillID::UNKNOWN_SKILL;
    }

    return skill_id;
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

std::string weapon_type_to_string(WeaponType weapon_type)
{
    switch (weapon_type)
    {
    case WeaponType::NONE:
        return "None";
    case WeaponType::GREATSWORD:
        return "Greatsword";
    case WeaponType::HAMMER:
        return "Hammer";
    case WeaponType::LONGBOW:
        return "Longbow";
    case WeaponType::RIFLE:
        return "Rifle";
    case WeaponType::SHORTBOW:
        return "Shortbow";
    case WeaponType::STAFF:
        return "Staff";
    case WeaponType::SPEAR:
        return "Spear";
    case WeaponType::TRIDENT:
        return "Trident";
    case WeaponType::HARPOON_GUN:
        return "Harpoon Gun";
    case WeaponType::AXE:
        return "Axe";
    case WeaponType::DAGGER:
        return "Dagger";
    case WeaponType::MACE:
        return "Mace";
    case WeaponType::PISTOL:
        return "Pistol";
    case WeaponType::SCEPTER:
        return "Scepter";
    case WeaponType::SWORD:
        return "Sword";
    case WeaponType::FOCUS:
        return "Focus";
    case WeaponType::SHIELD:
        return "Shield";
    case WeaponType::TORCH:
        return "Torch";
    case WeaponType::WARHORN:
        return "Warhorn";
    default:
        return "Unknown";
    }
}

std::string windows_key_to_string(WindowsKeys key)
{
    switch (key)
    {
    case WindowsKeys::LeftMouseBtn:
        return "Left Mouse";
    case WindowsKeys::RightMouseBtn:
        return "Right Mouse";
    case WindowsKeys::CtrlBrkPrcs:
        return "Ctrl+Break";
    case WindowsKeys::MidMouseBtn:
        return "Middle Mouse";
    case WindowsKeys::BackSpace:
        return "Backspace";
    case WindowsKeys::Tab:
        return "Tab";
    case WindowsKeys::Clear:
        return "Clear";
    case WindowsKeys::Enter:
        return "Enter";
    case WindowsKeys::Shift:
        return "Shift";
    case WindowsKeys::Control:
        return "Ctrl";
    case WindowsKeys::Alt:
        return "Alt";
    case WindowsKeys::Pause:
        return "Pause";
    case WindowsKeys::CapsLock:
        return "Caps Lock";
    case WindowsKeys::Escape:
        return "Esc";
    case WindowsKeys::Space:
        return "Space";
    case WindowsKeys::PageUp:
        return "Page Up";
    case WindowsKeys::PageDown:
        return "Page Down";
    case WindowsKeys::End:
        return "End";
    case WindowsKeys::Home:
        return "Home";
    case WindowsKeys::LeftArrow:
        return "Left Arrow";
    case WindowsKeys::UpArrow:
        return "Up Arrow";
    case WindowsKeys::RightArrow:
        return "Right Arrow";
    case WindowsKeys::DownArrow:
        return "Down Arrow";
    case WindowsKeys::Select:
        return "Select";
    case WindowsKeys::Print:
        return "Print";
    case WindowsKeys::Execute:
        return "Execute";
    case WindowsKeys::PrintScreen:
        return "Print Screen";
    case WindowsKeys::Inser:
        return "Insert";
    case WindowsKeys::Delete:
        return "Delete";
    case WindowsKeys::Help:
        return "Help";
    case WindowsKeys::Num0:
        return "0";
    case WindowsKeys::Num1:
        return "1";
    case WindowsKeys::Num2:
        return "2";
    case WindowsKeys::Num3:
        return "3";
    case WindowsKeys::Num4:
        return "4";
    case WindowsKeys::Num5:
        return "5";
    case WindowsKeys::Num6:
        return "6";
    case WindowsKeys::Num7:
        return "7";
    case WindowsKeys::Num8:
        return "8";
    case WindowsKeys::Num9:
        return "9";
    case WindowsKeys::A:
        return "A";
    case WindowsKeys::B:
        return "B";
    case WindowsKeys::C:
        return "C";
    case WindowsKeys::D:
        return "D";
    case WindowsKeys::E:
        return "E";
    case WindowsKeys::F:
        return "F";
    case WindowsKeys::G:
        return "G";
    case WindowsKeys::H:
        return "H";
    case WindowsKeys::I:
        return "I";
    case WindowsKeys::J:
        return "J";
    case WindowsKeys::K:
        return "K";
    case WindowsKeys::L:
        return "L";
    case WindowsKeys::M:
        return "M";
    case WindowsKeys::N:
        return "N";
    case WindowsKeys::O:
        return "O";
    case WindowsKeys::P:
        return "P";
    case WindowsKeys::Q:
        return "Q";
    case WindowsKeys::R:
        return "R";
    case WindowsKeys::S:
        return "S";
    case WindowsKeys::T:
        return "T";
    case WindowsKeys::U:
        return "U";
    case WindowsKeys::V:
        return "V";
    case WindowsKeys::W:
        return "W";
    case WindowsKeys::X:
        return "X";
    case WindowsKeys::Y:
        return "Y";
    case WindowsKeys::Z:
        return "Z";
    case WindowsKeys::LeftWin:
        return "Left Win";
    case WindowsKeys::RightWin:
        return "Right Win";
    case WindowsKeys::Apps:
        return "Apps";
    case WindowsKeys::Sleep:
        return "Sleep";
    case WindowsKeys::Numpad0:
        return "Numpad 0";
    case WindowsKeys::Numpad1:
        return "Numpad 1";
    case WindowsKeys::Numpad2:
        return "Numpad 2";
    case WindowsKeys::Numpad3:
        return "Numpad 3";
    case WindowsKeys::Numpad4:
        return "Numpad 4";
    case WindowsKeys::Numpad5:
        return "Numpad 5";
    case WindowsKeys::Numpad6:
        return "Numpad 6";
    case WindowsKeys::Numpad7:
        return "Numpad 7";
    case WindowsKeys::Numpad8:
        return "Numpad 8";
    case WindowsKeys::Numpad9:
        return "Numpad 9";
    case WindowsKeys::Multiply:
        return "Numpad *";
    case WindowsKeys::Add:
        return "Numpad +";
    case WindowsKeys::Separator:
        return "Separator";
    case WindowsKeys::Subtract:
        return "Numpad -";
    case WindowsKeys::Decimal:
        return "Numpad .";
    case WindowsKeys::Divide:
        return "Numpad /";
    case WindowsKeys::F1:
        return "F1";
    case WindowsKeys::F2:
        return "F2";
    case WindowsKeys::F3:
        return "F3";
    case WindowsKeys::F4:
        return "F4";
    case WindowsKeys::F5:
        return "F5";
    case WindowsKeys::F6:
        return "F6";
    case WindowsKeys::F7:
        return "F7";
    case WindowsKeys::F8:
        return "F8";
    case WindowsKeys::F9:
        return "F9";
    case WindowsKeys::F10:
        return "F10";
    case WindowsKeys::F11:
        return "F11";
    case WindowsKeys::F12:
        return "F12";
    case WindowsKeys::F13:
        return "F13";
    case WindowsKeys::F14:
        return "F14";
    case WindowsKeys::F15:
        return "F15";
    case WindowsKeys::F16:
        return "F16";
    case WindowsKeys::F17:
        return "F17";
    case WindowsKeys::F18:
        return "F18";
    case WindowsKeys::F19:
        return "F19";
    case WindowsKeys::F20:
        return "F20";
    case WindowsKeys::F21:
        return "F21";
    case WindowsKeys::F22:
        return "F22";
    case WindowsKeys::F23:
        return "F23";
    case WindowsKeys::F24:
        return "F24";
    case WindowsKeys::LeftShift:
        return "Left Shift";
    case WindowsKeys::RightShift:
        return "Right Shift";
    case WindowsKeys::LeftCtrl:
        return "Left Ctrl";
    case WindowsKeys::RightCtrl:
        return "Right Ctrl";
    case WindowsKeys::LeftMenu:
        return "Left Alt";
    case WindowsKeys::RightMenu:
        return "Right Alt";
    case WindowsKeys::Plus:
        return "+";
    case WindowsKeys::Comma:
        return ",";
    case WindowsKeys::Minus:
        return "-";
    case WindowsKeys::Period:
        return ".";
    default:
        return "Unknown";
    }
}
Keys windows_key_to_keys_enum(WindowsKeys key)
{
    switch (key)
    {
    case WindowsKeys::A:
        return Keys::A;
    case WindowsKeys::B:
        return Keys::B;
    case WindowsKeys::C:
        return Keys::C;
    case WindowsKeys::D:
        return Keys::D;
    case WindowsKeys::E:
        return Keys::E;
    case WindowsKeys::F:
        return Keys::F;
    case WindowsKeys::G:
        return Keys::G;
    case WindowsKeys::H:
        return Keys::H;
    case WindowsKeys::I:
        return Keys::I;
    case WindowsKeys::J:
        return Keys::J;
    case WindowsKeys::K:
        return Keys::K;
    case WindowsKeys::L:
        return Keys::L;
    case WindowsKeys::M:
        return Keys::M;
    case WindowsKeys::N:
        return Keys::N;
    case WindowsKeys::O:
        return Keys::O;
    case WindowsKeys::P:
        return Keys::P;
    case WindowsKeys::Q:
        return Keys::Q;
    case WindowsKeys::R:
        return Keys::R;
    case WindowsKeys::S:
        return Keys::S;
    case WindowsKeys::T:
        return Keys::T;
    case WindowsKeys::U:
        return Keys::U;
    case WindowsKeys::V:
        return Keys::V;
    case WindowsKeys::W:
        return Keys::W;
    case WindowsKeys::X:
        return Keys::X;
    case WindowsKeys::Y:
        return Keys::Y;
    case WindowsKeys::Z:
        return Keys::Z;
    case WindowsKeys::Num0:
        return Keys::ZERO;
    case WindowsKeys::Num1:
        return Keys::ONE;
    case WindowsKeys::Num2:
        return Keys::TWO;
    case WindowsKeys::Num3:
        return Keys::THREE;
    case WindowsKeys::Num4:
        return Keys::FOUR;
    case WindowsKeys::Num5:
        return Keys::FIVE;
    case WindowsKeys::Num6:
        return Keys::SIX;
    case WindowsKeys::Num7:
        return Keys::SEVEN;
    case WindowsKeys::Num8:
        return Keys::EIGHT;
    case WindowsKeys::Num9:
        return Keys::NINE;
    case WindowsKeys::F1:
        return Keys::F1;
    case WindowsKeys::F2:
        return Keys::F2;
    case WindowsKeys::F3:
        return Keys::F3;
    case WindowsKeys::F4:
        return Keys::F4;
    case WindowsKeys::F5:
        return Keys::F5;
    case WindowsKeys::F6:
        return Keys::F6;
    case WindowsKeys::F7:
        return Keys::F7;
    case WindowsKeys::Tab:
        return Keys::TAB;
    case WindowsKeys::LeftArrow:
        return Keys::LEFT_ARROW;
    case WindowsKeys::RightArrow:
        return Keys::RIGHT_ARROW;
    case WindowsKeys::LeftCtrl:
        return Keys::LEFT_CTRL;
    case WindowsKeys::LeftShift:
        return Keys::LEFT_SHIFT;
    case WindowsKeys::LeftMenu:
        return Keys::LEFT_ALT;
    case WindowsKeys::CapsLock:
        return Keys::CAPS;
    case WindowsKeys::Minus:
        return Keys::MINUS;
    case WindowsKeys::Add:
        return Keys::NUM_ADD;
    default:
        return Keys::NONE;
    }
}
