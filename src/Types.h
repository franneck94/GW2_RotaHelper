#pragma once

#include <cstdint>
#include <d3d11.h>
#include <filesystem>
#include <list>
#include <map>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "arcdps/ArcDPS.h"

#include "Defines.h"
#include "SkillIDs.h"

struct EvCombatDataPersistent
{
    std::string SrcName;
    uintptr_t SrcID;
    uint32_t SrcProfession;
    uint32_t SrcSpecialization;
    std::string SkillName;
    SkillID SkillID;
    uint64_t EventID;
    bool RepeatedSkill;
};

struct EvCombatData
{
    ArcDPS::CombatEvent *ev;
    ArcDPS::AgentShort *src;
    ArcDPS::AgentShort *dst;
    char *skillname;
    uint64_t id;
    uint64_t revision;
};

struct EvAgentUpdate
{
    char account[64];     // dst->name  = account name
    char character[64];   // src->name  = character name
    uintptr_t id;         // src->id    = agent id
    uintptr_t instanceId; // dst->id    = instance id (per map)
    uint32_t added;       // src->prof  = is new agent
    uint32_t target;      // src->elite = is new targeted agent
    uint32_t Self;        // dst->Self  = is Self
    uint32_t prof;        // dst->prof  = profession / core spec
    uint32_t elite;       // dst->elite = elite spec
    uint16_t team;        // src->team  = team
    uint16_t subgroup;    // dst->team  = subgroup
};

struct BenchFileInfo
{
    std::filesystem::path full_path;
    std::filesystem::path relative_path;
    std::string display_name;
    bool is_directory_header;

    BenchFileInfo(const std::filesystem::path &full, const std::filesystem::path &relative, bool is_header = false);
};

struct SkillState
{
    bool is_current;
    bool is_last;
    bool is_auto_attack;
};

struct LogSkillInfo
{
    std::string name;
    std::string icon_url;
};

using LogSkillInfoMap = std::map<int, LogSkillInfo>;

using LogDataTypes = std::variant<int, float, bool, std::string>;

struct IntNode
{
    std::map<std::string, IntNode> children;
    std::optional<LogDataTypes> value;
};

enum class SkillSlot
{
    NONE,
    WEAPON_1,
    WEAPON_2,
    WEAPON_3,
    WEAPON_4,
    WEAPON_5,
    HEAL,
    UTILITY_1,
    UTILITY_2,
    UTILITY_3,
    ELITE,
    PROFESSION_1,
    PROFESSION_2,
    PROFESSION_3,
    PROFESSION_4,
    PROFESSION_5,
    PROFESSION_6,
    PROFESSION_7,
};

enum class WeaponType
{
    NONE = 0,
    GREATSWORD = 1,
    HAMMER = 2,
    LONGBOW = 3,
    RIFLE = 4,
    SHORTBOW = 5,
    STAFF = 6,
    SPEAR = 7,
    TRIDENT = 8,
    HARPOON_GUN = 9,
    AXE = 10,
    DAGGER = 11,
    MACE = 12,
    PISTOL = 13,
    SCEPTER = 14,
    SWORD = 15,
    FOCUS = 16,
    SHIELD = 17,
    TORCH = 18,
    WARHORN = 19,
};

struct SkillData
{
    int icon_id;
    SkillID skill_id;
    std::string name;
    float recharge_time;
    float recharge_time_with_alacrity;
    float cast_time;
    float cast_time_with_quickness;
    bool is_auto_attack;
    bool is_weapon_skill;
    bool is_utility_skill;
    bool is_elite_skill;
    bool is_heal_skill;
    bool is_profession_skill;
    SkillSlot skill_type;
    WeaponType weapon_type;
};

struct RotationStep
{
    float time_of_cast;
    float duration_ms;
    SkillData skill_data;
    bool is_special_skill;
};

struct RotationSkill
{
    SkillID skill_id;
    std::string name;
    ID3D11ShaderResourceView *texture;
};

using RotationSteps = std::vector<RotationStep>;
using RotationStepsList = std::list<RotationStep>;
using SkillDataMap = std::map<SkillID, SkillData>;

enum class ProfessionID : uint32_t
{
    UNKNOWN = 0,
    GUARDIAN = 1,
    WARRIOR = 2,
    ENGINEER = 3,
    RANGER = 4,
    THIEF = 5,
    ELEMENTALIST = 6,
    MESMER = 7,
    NECROMANCER = 8,
    REVENANT = 9,
};

enum class EliteSpecID : uint32_t
{
    // Elementalist
    Catalyst = 67,
    Evoker = 80,
    Tempest = 48,
    Weaver = 56,

    // Engineer
    Amalgam = 75,
    Holosmith = 57,
    Mechanist = 70,
    Scrapper = 43,

    // Guardian
    Dragonhunter = 27,
    Firebrand = 62,
    Luminary = 81,
    Willbender = 65,

    // Mesmer
    Chronomancer = 40,
    Mirage = 59,
    Troubadour = 73,
    Virtuoso = 66,

    // Necromancer
    Harbinger = 64,
    Reaper = 34,
    Ritualist = 76,
    Scourge = 60,

    // Ranger
    Druid = 5,
    Galeshot = 78,
    Soulbeast = 55,
    Untamed = 72,

    // Revenant
    Conduit = 79,
    Herald = 52,
    Renegade = 63,
    Vindicator = 69,

    // Thief
    Antiquary = 77,
    Daredevil = 7,
    Deadeye = 58,
    Specter = 71,

    // Warrior
    Berserker = 18,
    Bladesworn = 68,
    Paragon = 74,
    Spellbreaker = 61,

    // Default/Unknown
    Unknown = 0
};

struct MetaData
{
    double overall_dps;
    std::string name;
    std::string url;
    std::string benchmark_type;
    std::string profession;
    ProfessionID profession_id;
    std::string elite_spec;
    EliteSpecID elite_spec_id;
    std::string build_type;
    std::string url_name;
    std::string dps_report_url;
    std::string html_file_path;
};

struct SkillKeyMapping
{
    int skill_7;
    int skill_8;
    int skill_9;
};

enum class Device
{
    KEYBOARD,
    MOUSE,
};

enum class MouseKeys
{
    MOUSE1 = 0, // Check
    MOUSE2 = 1, // Check
    MOUSE3 = 2, // Check
    MOUSE4 = 3,
    MOUSE5 = 4,
};

enum class Keys
{
    NONE,
    LEFT_CTRL = 1,
    LEFT_SHIFT = 2,
    AE = 3,
    CAPS = 5,
    MINUS = 7,
    EQUAL = 8,
    ZIRUMFLEX = 17,
    TAB = 22,
    LEFT_ARROW = 29,
    RIGHT_ARROW = 30,
    F1 = 32,
    F2 = 33,
    F3 = 34,
    F4 = 35,
    F5 = 36,
    F6 = 37,
    F7 = 38,
    ONE = 49,
    TWO = 50,
    THREE = 51,
    FOUR = 52,
    FIVE = 53,
    SIX = 54,
    SEVEN = 55,
    EIGHT = 56,
    NINE = 57,
    ZERO = 48,
    A = 65,
    B = 66,
    C = 67,
    D = 68,
    E = 69,
    F = 70,
    G = 71,
    H = 72,
    I = 73,
    J = 74,
    K = 75,
    L = 76,
    M = 77,
    N = 78,
    O = 79,
    P = 80,
    Q = 81,
    R = 82,
    S = 83,
    T = 84,
    U = 85,
    V = 86,
    W = 87,
    X = 88,
    Y = 89,
    Z = 90,
    NUM_ADD = 91,
    NUM_1 = 96,
    NUM_2 = 97,
    NUM_3 = 98,
    NUM_4 = 99,
    NUM_5 = 100,
    NUM_6 = 101,
    NUM_7 = 102,
    NUM_8 = 103,
    NUM_9 = 104,
    NUM_RET = 105,
    SMALLER = 111,
    LEFT_ALT = 202,
};

enum class WindowsKeys
{
    LeftMouseBtn = 0x01,  //Left mouse button
    RightMouseBtn = 0x02, //Right mouse button
    CtrlBrkPrcs = 0x03,   //Control-break processing
    MidMouseBtn = 0x04,   //Middle mouse button
    BackSpace = 0x08, //Backspace key
    Tab = 0x09,       //Tab key
    Clear = 0x0C, //Clear key
    Enter = 0x0D, //Enter or Return key
    Shift = 0x10,    //Shift key
    Control = 0x11,  //Ctrl key
    Alt = 0x12,      //Alt key
    Pause = 0x13,    //Pause key
    CapsLock = 0x14, //Caps lock key
    Escape = 0x1B, //Esc key
    Space = 0x20,       //Space bar
    PageUp = 0x21,      //Page up key
    PageDown = 0x22,    //Page down key
    End = 0x23,         //End key
    Home = 0x24,        //Home key
    LeftArrow = 0x25,   //Left arrow key
    UpArrow = 0x26,     //Up arrow key
    RightArrow = 0x27,  //Right arrow key
    DownArrow = 0x28,   //Down arrow key
    Select = 0x29,      //Select key
    Print = 0x2A,       //Print key
    Execute = 0x2B,     //Execute key
    PrintScreen = 0x2C, //Print screen key
    Inser = 0x2D,       //Insert key
    Delete = 0x2E,      //Delete key
    Help = 0x2F,        //Help key
    Num0 = 0x30, //Top row 0 key (Matches '0')
    Num1 = 0x31, //Top row 1 key (Matches '1')
    Num2 = 0x32, //Top row 2 key (Matches '2')
    Num3 = 0x33, //Top row 3 key (Matches '3')
    Num4 = 0x34, //Top row 4 key (Matches '4')
    Num5 = 0x35, //Top row 5 key (Matches '5')
    Num6 = 0x36, //Top row 6 key (Matches '6')
    Num7 = 0x37, //Top row 7 key (Matches '7')
    Num8 = 0x38, //Top row 8 key (Matches '8')
    Num9 = 0x39, //Top row 9 key (Matches '9')
    A = 0x41, //A key (Matches 'A')
    B = 0x42, //B key (Matches 'B')
    C = 0x43, //C key (Matches 'C')
    D = 0x44, //D key (Matches 'D')
    E = 0x45, //E key (Matches 'E')
    F = 0x46, //F key (Matches 'F')
    G = 0x47, //G key (Matches 'G')
    H = 0x48, //H key (Matches 'H')
    I = 0x49, //I key (Matches 'I')
    J = 0x4A, //J key (Matches 'J')
    K = 0x4B, //K key (Matches 'K')
    L = 0x4C, //L key (Matches 'L')
    M = 0x4D, //M key (Matches 'M')
    N = 0x4E, //N key (Matches 'N')
    O = 0x4F, //O key (Matches 'O')
    P = 0x50, //P key (Matches 'P')
    Q = 0x51, //Q key (Matches 'Q')
    R = 0x52, //R key (Matches 'R')
    S = 0x53, //S key (Matches 'S')
    T = 0x54, //T key (Matches 'T')
    U = 0x55, //U key (Matches 'U')
    V = 0x56, //V key (Matches 'V')
    W = 0x57, //W key (Matches 'W')
    X = 0x58, //X key (Matches 'X')
    Y = 0x59, //Y key (Matches 'Y')
    Z = 0x5A, //Z key (Matches 'Z')
    LeftWin = 0x5B,  //Left windows key
    RightWin = 0x5C, //Right windows key
    Apps = 0x5D,     //Applications key
    Sleep = 0x5F, //Computer sleep key
    Numpad0 = 0x60,   //Numpad 0
    Numpad1 = 0x61,   //Numpad 1
    Numpad2 = 0x62,   //Numpad 2
    Numpad3 = 0x63,   //Numpad 3
    Numpad4 = 0x64,   //Numpad 4
    Numpad5 = 0x65,   //Numpad 5
    Numpad6 = 0x66,   //Numpad 6
    Numpad7 = 0x67,   //Numpad 7
    Numpad8 = 0x68,   //Numpad 8
    Numpad9 = 0x69,   //Numpad 9
    Multiply = 0x6A,  //Multiply key
    Add = 0x6B,       //Add key
    Separator = 0x6C, //Separator key
    Subtract = 0x6D,  //Subtract key
    Decimal = 0x6E,   //Decimal key
    Divide = 0x6F,    //Divide key
    F1 = 0x70,        //F1
    F2 = 0x71,        //F2
    F3 = 0x72,        //F3
    F4 = 0x73,        //F4
    F5 = 0x74,        //F5
    F6 = 0x75,        //F6
    F7 = 0x76,        //F7
    F8 = 0x77,        //F8
    F9 = 0x78,        //F9
    F10 = 0x79,       //F10
    F11 = 0x7A,       //F11
    F12 = 0x7B,       //F12
    F13 = 0x7C,       //F13
    F14 = 0x7D,       //F14
    F15 = 0x7E,       //F15
    F16 = 0x7F,       //F16
    F17 = 0x80,       //F17
    F18 = 0x81,       //F18
    F19 = 0x82,       //F19
    F20 = 0x83,       //F20
    F21 = 0x84,       //F21
    F22 = 0x85,       //F22
    F23 = 0x86,       //F23
    F24 = 0x87,       //F24
    LeftShift = 0xA0,  //Left shift key
    RightShift = 0xA1, //Right shift key
    LeftCtrl = 0xA2,   //Left control key
    RightCtrl = 0xA3,  //Right control key
    LeftMenu = 0xA4,   //Left menu key
    RightMenu = 0xA5,  //Right menu
    Plus = 0xBB,   //Plus key
    Comma = 0xBC,  //Comma key
    Minus = 0xBD,  //Minus key
    Period = 0xBE, //Period key
};

enum class Modifiers
{
    NONE = 0,
    SHIFT = 1,
    CTRL = 2, // Checked
    RALT = 3,// Checked
    ALT = 4, // Checked
    RCTRL = 6,// Checked
};

struct KeybindInfo
{
    std::string action_name;
    Keys button = Keys::NONE;
    Device device = Device::KEYBOARD;
    Modifiers modifier = Modifiers::NONE;
};

struct SkillRules
{
    const std::set<std::string_view> &skills_substr_weapon_swap_like;
    const std::set<std::string_view> &skills_match_weapon_swap_like;
    const std::set<std::string_view> &skills_substr_to_drop;
    const std::set<std::string_view> &skills_match_to_drop;
    const std::set<std::string_view> &special_substr_to_gray_out;
    const std::set<std::string_view> &special_match_to_gray_out_names;
    const std::set<SkillID> &special_match_to_gray_out;
    const std::set<std::string_view> &special_substr_to_remove_duplicates_names;
    const std::set<SkillID> &special_substr_to_remove_duplicates;
    const std::set<std::string_view> &easy_mode_drop_match_name;
    const std::set<SkillID> &easy_mode_drop_match;
    const std::map<std::string_view, std::set<SkillID>> &class_map_easy_mode_match_to_gray_out;
    const std::map<std::string_view, std::set<SkillID>> &class_map_special_match_to_gray_out;
    const std::map<std::string_view, std::set<SkillID>> &class_map_easy_mode_drop_match;
};

enum class DownloadState : uint8_t
{
    NOT_STARTED,
    STARTED,
    FINISHED,
    FAILED,
    NO_UPDATE_NEEDED,
};

struct RotaSkillWindow
{
    RotationStep curr_rota_skill;
    RotationStep next_rota_skill;
    bool check_for_next_skill;
    RotationStep next_next_rota_skill;
    bool check_for_next_next_skill;
    RotationStep next_next_next_rota_skill;
    bool check_for_next_next_next_skill;
};

struct SkillDetectionTimers
{
    std::chrono::steady_clock::time_point time_of_last_next_skill_check = std::chrono::steady_clock::now();
    std::chrono::steady_clock::time_point time_of_last_next_next_skill_check = std::chrono::steady_clock::now();
    std::chrono::steady_clock::time_point time_of_last_next_next_next_skill_check = std::chrono::steady_clock::now();
    std::chrono::steady_clock::time_point time_of_last_aa_skip = std::chrono::steady_clock::now();
    std::chrono::steady_clock::time_point time_of_last_pop = std::chrono::steady_clock::now();
    bool is_first_check_for_next = true;
    bool is_first_check_for_next_next = true;
    bool is_first_check_for_next_next_next = true;
};

enum class BuildCategory
{
    RED_CROSSED,
    ORANGE_CROSSED,
    GREEN_TICKED,
    YELLOW_TICKED,
    UNTESTED
};
