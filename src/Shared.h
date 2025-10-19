#ifndef SHARED_H
#define SHARED_H

#include <array>
#include <filesystem>
#include <string>

#include "arcdps/ArcDPS.h"
#include "mumble/Mumble.h"
#include "nexus/Nexus.h"
#include "rtapi/RTAPI.hpp"

#include "Render.h"
#include "Types.h"

extern AddonAPI *APIDefs;
extern NexusLinkData *NexusLink;
extern RTAPI::RealTimeData *RTAPIData;
extern std::string AccountName;
extern ArcDPS::Exports ArcExports;
extern std::filesystem::path logpath;
extern Render render;
extern RotationRun rotation_run;
extern TextureMap texture_map;
extern std::filesystem::path SettingsPath;
extern float SKILL_ICON_SIZE;

#endif
