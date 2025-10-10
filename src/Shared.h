#ifndef SHARED_H
#define SHARED_H

#include <array>
#include <string>
#include <filesystem>

#include "arcdps/ArcDPS.h"
#include "mumble/Mumble.h"
#include "nexus/Nexus.h"
#include "rtapi/RTAPI.hpp"

#include "Types.h"
#include "Render.h"

extern AddonAPI *APIDefs;
extern NexusLinkData *NexusLink;
extern RTAPI::RealTimeData *RTAPIData;
extern const char *ADDON_NAME;
extern std::string AccountName;
extern ArcDPS::Exports ArcExports;
extern std::array<EvCombatDataPersistent, 10> combat_buffer;
extern size_t combat_buffer_index;
extern size_t prev_combat_buffer_index;
extern std::filesystem::path logpath;
extern Render render;

#endif
