#include <filesystem>
#include <fstream>
#include <string>

#include "Settings.h"
#include "Shared.h"

const char *SHOW_WINDOW = "ShowWindow";
const char *FILTER_BUFFER = "FilterBuffer";
const char *SHOW_SKILL_NAME = "ShowSkillName";
const char *SHOW_SKILL_TIME = "ShowSkillTime";
const char *HORIZONTAL_SKILL_LAYOUT = "HorizontalSkillLayout";
const char *SHOW_WEAPON_SWAP = "ShowWeaponSwap";
const char *SHOW_KEYBIND = "ShowKeybind";
const char *XML_SETTINGS_FILE = "XmlSettingsFile";
const char *STRICT_MODE_FOR_SKILL_DETECTION = "StrictModeForSkillDetection";
const char *EASY_SKILL_MODE = "EasySkillMode";

namespace Settings
{
std::mutex Mutex;
json Settings = json::object();

void Load(std::filesystem::path aPath)
{
    if (!std::filesystem::exists(aPath))
    {
        return;
    }

    Settings::Mutex.lock();
    {
        try
        {
            std::ifstream file(aPath);
            Settings = json::parse(file);
            file.close();
        }
        catch (json::parse_error &ex)
        {
            Globals::APIDefs->Log(ELogLevel_WARNING, "GW2RotaHelper", "Settings.json could not be parsed.");
            Globals::APIDefs->Log(ELogLevel_WARNING, "GW2RotaHelper", ex.what());
        }
    }
    Settings::Mutex.unlock();

    /* Widget */
    if (!Settings[SHOW_WINDOW].is_null())
    {
        Settings[SHOW_WINDOW].get_to<bool>(ShowWindow);
    }
    if (!Settings[FILTER_BUFFER].is_null())
    {
        Settings[FILTER_BUFFER].get_to<std::string>(FilterBuffer);
    }
    if (!Settings[SHOW_SKILL_NAME].is_null())
    {
        Settings[SHOW_SKILL_NAME].get_to<bool>(ShowSkillName);
    }
    if (!Settings[SHOW_SKILL_TIME].is_null())
    {
        Settings[SHOW_SKILL_TIME].get_to<bool>(ShowSkillTime);
    }
    if (!Settings[HORIZONTAL_SKILL_LAYOUT].is_null())
    {
        Settings[HORIZONTAL_SKILL_LAYOUT].get_to<bool>(HorizontalSkillLayout);
    }
    if (!Settings[SHOW_WEAPON_SWAP].is_null())
    {
        Settings[SHOW_WEAPON_SWAP].get_to<bool>(ShowWeaponSwap);
    }
    if (!Settings[SHOW_KEYBIND].is_null())
    {
        Settings[SHOW_KEYBIND].get_to<bool>(ShowKeybind);
    }
    if (!Settings[STRICT_MODE_FOR_SKILL_DETECTION].is_null())
    {
        Settings[STRICT_MODE_FOR_SKILL_DETECTION].get_to<bool>(StrictModeForSkillDetection);
    }
    if (!Settings[EASY_SKILL_MODE].is_null())
    {
        Settings[EASY_SKILL_MODE].get_to<bool>(EasySkillMode);
    }
    if (!Settings[XML_SETTINGS_FILE].is_null())
    {
        auto _XmlSettingsPath = std::string{};
        Settings[XML_SETTINGS_FILE].get_to<std::string>(_XmlSettingsPath);
        XmlSettingsPath = std::filesystem::path{_XmlSettingsPath};
    }
}

void Save(std::filesystem::path aPath)
{
    Settings::Mutex.lock();
    {
        Settings[SHOW_WINDOW] = ShowWindow;
        Settings[FILTER_BUFFER] = FilterBuffer;
        Settings[SHOW_SKILL_NAME] = ShowSkillName;
        Settings[SHOW_SKILL_TIME] = ShowSkillTime;
        Settings[HORIZONTAL_SKILL_LAYOUT] = HorizontalSkillLayout;
        Settings[SHOW_WEAPON_SWAP] = ShowWeaponSwap;
        Settings[SHOW_KEYBIND] = ShowKeybind;
        Settings[XML_SETTINGS_FILE] = XmlSettingsPath.string();
        Settings[STRICT_MODE_FOR_SKILL_DETECTION] = StrictModeForSkillDetection;
        Settings[EASY_SKILL_MODE] = EasySkillMode;

        std::ofstream file(aPath);
        file << Settings.dump(1, '\t') << std::endl;
        file.close();
    }
    Settings::Mutex.unlock();
}

void ToggleShowWindow(std::filesystem::path SettingsPath)
{
    ShowWindow = !ShowWindow;
    Settings[SHOW_WINDOW] = ShowWindow;
    Save(Globals::SettingsPath);
}

bool ShowWindow = true;
std::string FilterBuffer;
bool ShowSkillName = true;
bool ShowSkillTime = true;
bool HorizontalSkillLayout = false;
bool ShowWeaponSwap = false;
bool ShowKeybind = false;
bool StrictModeForSkillDetection = false;
bool EasySkillMode = false;
std::filesystem::path XmlSettingsPath;
} // namespace Settings
