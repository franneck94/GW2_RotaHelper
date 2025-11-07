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

namespace Globals
{
extern AddonAPI *APIDefs;
extern NexusLinkData *NexusLink;
extern RTAPI::RealTimeData *RTAPIData;
extern std::string AccountName;
extern Mumble::Data *MumbleData;
extern ArcDPS::Exports ArcExports;

extern std::filesystem::path Logpath;
extern RenderType Render;
extern RotationLogType RotationRun;
extern TextureMapType TextureMap;
extern std::filesystem::path SettingsPath;
extern float SkillIconSize;
extern std::string VersionString;

extern Mumble::Identity Identity;

extern bool IsSameCast;
extern std::map<uint64_t, std::chrono::steady_clock::time_point> SkillLastTimeCast;
}; // namespace Globals

#endif
