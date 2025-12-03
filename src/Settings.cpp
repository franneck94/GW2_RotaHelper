#include <filesystem>
#include <fstream>
#include <string>

#include "Settings.h"
#include "Shared.h"

const char *SHOW_WINDOW = "ShowWindow";
const char *FILTER_BUFFER = "FilterBuffer";
const char *SHOW_WEAPON_SWAP = "ShowWeaponSwap";
const char *SHOW_KEYBIND = "ShowKeybind";
const char *XML_SETTINGS_FILE = "XmlSettingsFile";
const char *STRICT_MODE_FOR_SKILL_DETECTION = "StrictModeForSkillDetection";
const char *EASY_SKILL_MODE = "EasySkillMode";
const char *VERSION_OF_LAST_BENCH_FILES_UPDATE = "VersionOfLastBenchFilesUpdate";
const char *SKIP_BENCH_FILE_UPDATE = "SkipBenchFileUpdate";
const char *BENCH_UPDATE_FAILED_BEFORE = "BenchUpdateFailedBefore";

namespace Settings
{
std::mutex Mutex;
json Settings = json::object();

void Load(std::filesystem::path aPath)
{
    if (!std::filesystem::exists(aPath))
        return;

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
        Settings[SHOW_WINDOW].get_to<bool>(ShowWindow);
    if (!Settings[FILTER_BUFFER].is_null())
    {
        auto filter_str = Settings[FILTER_BUFFER].get<std::string>();
        strncpy_s(FilterBuffer, sizeof(FilterBuffer), filter_str.c_str(), _TRUNCATE);
    }
    if (!Settings[SHOW_WEAPON_SWAP].is_null())
        Settings[SHOW_WEAPON_SWAP].get_to<bool>(ShowWeaponSwap);
    if (!Settings[SHOW_KEYBIND].is_null())
        Settings[SHOW_KEYBIND].get_to<bool>(ShowKeybind);
    if (!Settings[STRICT_MODE_FOR_SKILL_DETECTION].is_null())
        Settings[STRICT_MODE_FOR_SKILL_DETECTION].get_to<bool>(StrictModeForSkillDetection);
    if (!Settings[EASY_SKILL_MODE].is_null())
        Settings[EASY_SKILL_MODE].get_to<bool>(EasySkillMode);
    if (!Settings[VERSION_OF_LAST_BENCH_FILES_UPDATE].is_null())
        Settings[VERSION_OF_LAST_BENCH_FILES_UPDATE].get_to<std::string>(VersionOfLastBenchFilesUpdate);
    if (!Settings[XML_SETTINGS_FILE].is_null())
    {
        auto _XmlSettingsPath = std::string{};
        Settings[XML_SETTINGS_FILE].get_to<std::string>(_XmlSettingsPath);
        XmlSettingsPath = std::filesystem::path{_XmlSettingsPath};
    }
    if (!Settings[SKIP_BENCH_FILE_UPDATE].is_null())
        Settings[SKIP_BENCH_FILE_UPDATE].get_to<bool>(SkipBenchFileUpdate);
    if (!Settings[BENCH_UPDATE_FAILED_BEFORE].is_null())
        Settings[BENCH_UPDATE_FAILED_BEFORE].get_to<bool>(BenchUpdateFailedBefore);
}

void Save(std::filesystem::path aPath)
{
    Settings::Mutex.lock();
    {
        Settings[SHOW_WINDOW] = ShowWindow;
        Settings[FILTER_BUFFER] = std::string(FilterBuffer);
        Settings[SHOW_WEAPON_SWAP] = ShowWeaponSwap;
        Settings[SHOW_KEYBIND] = ShowKeybind;
        Settings[XML_SETTINGS_FILE] = XmlSettingsPath.string();
        Settings[STRICT_MODE_FOR_SKILL_DETECTION] = StrictModeForSkillDetection;
        Settings[EASY_SKILL_MODE] = EasySkillMode;
        Settings[VERSION_OF_LAST_BENCH_FILES_UPDATE] = VersionOfLastBenchFilesUpdate;
        Settings[SKIP_BENCH_FILE_UPDATE] = SkipBenchFileUpdate;
        Settings[BENCH_UPDATE_FAILED_BEFORE] = BenchUpdateFailedBefore;

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
char FilterBuffer[50] = "";
bool ShowWeaponSwap = false;
bool ShowKeybind = false;
bool StrictModeForSkillDetection = false;
bool EasySkillMode = false;
std::filesystem::path XmlSettingsPath;
std::string VersionOfLastBenchFilesUpdate = "0.1.0.0";
bool SkipBenchFileUpdate = false;
bool BenchUpdateFailedBefore = false;
} // namespace Settings
