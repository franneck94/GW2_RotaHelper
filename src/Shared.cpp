#include <string>

#include "arcdps/ArcDPS.h"

#include "Shared.h"

AddonAPI *APIDefs = nullptr;
NexusLinkData *NexusLink = nullptr;
RTAPI::RealTimeData *RTAPIData = nullptr;
std::string AccountName;
ArcDPS::Exports ArcExports = {};

const char *KB_TOGGLE_GW2RotaHelper = "KB_TOGGLE_GW2RotaHelper";
const char *ADDON_NAME = "GW2RotaHelper";
