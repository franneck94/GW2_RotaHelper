#include <filesystem>
#include <fstream>
#include <string>

#include "Settings.h"
#include "Shared.h"

const char *SHOW_WINDOW = "ShowWindow";
const char *FILTER_BUFFER = "FilterBuffer";
const char *SHOW_SKILL_NAME = "ShowSkillName";
const char *SHOW_SKILL_TIME = "ShowSkillTime";

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
            APIDefs->Log(ELogLevel_WARNING,
                         "GW2RotaHelper",
                         "Settings.json could not be parsed.");
            APIDefs->Log(ELogLevel_WARNING, "GW2RotaHelper", ex.what());
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
}

void Save(std::filesystem::path aPath)
{
    Settings::Mutex.lock();
    {
        Settings[SHOW_WINDOW] = ShowWindow;
        Settings[FILTER_BUFFER] = FilterBuffer;
        Settings[SHOW_SKILL_NAME] = ShowSkillName;
        Settings[SHOW_SKILL_TIME] = ShowSkillTime;

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
    Save(SettingsPath);
}

bool ShowWindow = true;
std::string FilterBuffer;
bool ShowSkillName = true;
bool ShowSkillTime = true;
} // namespace Settings
