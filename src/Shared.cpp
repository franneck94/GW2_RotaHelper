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

std::array<EvCombatDataPersistent, 10> combat_buffer = {};
size_t combat_buffer_index = 0;
size_t prev_combat_buffer_index = 0;

std::filesystem::path logpath;

RotationRun rotation_run{};
TextureMap texture_map{};

Render render{};
