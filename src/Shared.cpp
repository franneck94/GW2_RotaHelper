#include <string>
#include <filesystem>

#include "arcdps/ArcDPS.h"

#include "Shared.h"

AddonAPI *APIDefs = nullptr;
NexusLinkData *NexusLink = nullptr;
RTAPI::RealTimeData *RTAPIData = nullptr;
std::string AccountName;
ArcDPS::Exports ArcExports = {};
const char *ADDON_NAME = "GW2RotaHelper";

std::array<EvCombatDataPersistent, 100> combat_buffer = {};
size_t combat_buffer_index = 0;

std::filesystem::path logpath;
