#include <filesystem>
#include <fstream>

#include "Settings.h"
#include "Shared.h"

const char *SHOW_WINDOW = "ShowWindow";

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
                APIDefs->Log(ELogLevel_WARNING, "GW2TP", "Settings.json could not be parsed.");
                APIDefs->Log(ELogLevel_WARNING, "GW2TP", ex.what());
            }
        }
        Settings::Mutex.unlock();

        /* Widget */
        if (!Settings[SHOW_WINDOW].is_null())
        {
            Settings[SHOW_WINDOW].get_to<bool>(ShowWindow);
        }
    }

    void Save(std::filesystem::path aPath)
    {
        Settings::Mutex.lock();
        {
            std::ofstream file(aPath);
            file << Settings.dump(1, '\t') << std::endl;
            file.close();
        }
        Settings::Mutex.unlock();
    }

    bool ShowWindow = true;
}
