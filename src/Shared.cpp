#include <array>
#include <filesystem>
#include <string>

#include "arcdps/ArcDPS.h"

#include "Render.h"
#include "Shared.h"
#include "Types.h"

AddonAPI *APIDefs = nullptr;
NexusLinkData *NexusLink = nullptr;
RTAPI::RealTimeData *RTAPIData = nullptr;
std::string AccountName;
ArcDPS::Exports ArcExports = {};

std::filesystem::path logpath;

RotationRun rotation_run{};
TextureMap texture_map{};

Render render{};

std::filesystem::path SettingsPath;

float SKILL_ICON_SIZE = 28.0F;
