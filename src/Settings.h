#ifndef SETTINGS_H
#define SETTINGS_H

#include <filesystem>
#include <mutex>
#include <string>

#include "nlohmann/json.hpp"
using json = nlohmann::json;

#include "mumble/Mumble.h"
#include "nexus/Nexus.h"
#include "rtapi/RTAPI.hpp"

extern const char *SHOW_WINDOW;
extern const char *FILTER_BUFFER;
extern const char *SHOW_WEAPON_SWAP;
extern const char *SHOW_KEYBIND;
extern const char *XML_SETTINGS_FILE;
extern const char *STRICT_MODE_FOR_SKILL_DETECTION;
extern const char *EASY_SKILL_MODE;
extern const char *VERSION_OF_LAST_BENCH_FILES_UPDATE;

namespace Settings
{
extern std::mutex Mutex;
extern json Settings;

void Load(std::filesystem::path aPath);
void Save(std::filesystem::path aPath);
void ToggleShowWindow(std::filesystem::path SettingsPath);

extern bool ShowWindow;
extern char FilterBuffer[50];
extern bool ShowWeaponSwap;
extern bool ShowKeybind;
extern bool StrictModeForSkillDetection;
extern bool EasySkillMode;
extern std::filesystem::path XmlSettingsPath;
extern std::string VersionOfLastBenchFilesUpdate;
} // namespace Settings

#endif
