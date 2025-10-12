#include <string>
#include <filesystem>

#include "arcdps/ArcDPS.h"

#include "Shared.h"
#include "Render.h"
#include "Types.h"

AddonAPI *APIDefs = nullptr;
NexusLinkData *NexusLink = nullptr;
RTAPI::RealTimeData *RTAPIData = nullptr;
std::string AccountName;
ArcDPS::Exports ArcExports = {};
const char *ADDON_NAME = "GW2RotaHelper";
const char *KB_TOGGLE_GW2_RotaHelper = "KB_TOGGLE_GW2_RotaHelper";

std::array<EvCombatDataPersistent, 10> combat_buffer = {};
size_t combat_buffer_index = 0;
size_t prev_combat_buffer_index = 0;

std::filesystem::path logpath;

Render render{};
