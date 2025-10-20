#ifndef SHARED_H
#define SHARED_H

#include <array>
#include <filesystem>
#include <string>

#include "arcdps/ArcDPS.h"
#include "mumble/Mumble.h"
#include "nexus/Nexus.h"
#include "rtapi/RTAPI.hpp"

#include "MemoryReader.h"
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
extern RotationRunType RotationRun;
extern TextureMapType TextureMap;
extern std::filesystem::path SettingsPath;
extern float SkillIconSize;
extern std::string VersionString;

#ifdef _DEBUG
extern GW2MemoryReader MemoryReader;
#endif
}; // namespace Globals

#endif
