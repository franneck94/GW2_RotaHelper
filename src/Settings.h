#ifndef SETTINGS_H
#define SETTINGS_H

#include <mutex>
#include <string>

#include "nlohmann/json.hpp"
using json = nlohmann::json;

#include "mumble/Mumble.h"
#include "nexus/Nexus.h"
#include "rtapi/RTAPI.hpp"

extern const char *SHOW_WINDOW;
extern const char *FILTER_BUFFER;
extern const char *SHOW_SKILL_NAME;
extern const char *SHOW_SKILL_TIME;
extern const char *HORIZONTAL_SKILL_LAYOUT;

namespace Settings
{
extern std::mutex Mutex;
extern json Settings;

/* Loads the settings. */
void Load(std::filesystem::path aPath);
/* Saves the settings. */
void Save(std::filesystem::path aPath);

void ToggleShowWindow(std::filesystem::path SettingsPath);

extern bool ShowWindow;
extern std::string FilterBuffer;
extern bool ShowSkillName;
extern bool ShowSkillTime;
extern bool HorizontalSkillLayout;
} // namespace Settings

#endif
