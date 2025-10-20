#include <array>
#include <filesystem>
#include <string>

#include "arcdps/ArcDPS.h"

#include "Render.h"
#include "Shared.h"
#include "Types.h"
#include "Version.h"

namespace Globals
{
AddonAPI *APIDefs = nullptr;
NexusLinkData *NexusLink = nullptr;
RTAPI::RealTimeData *RTAPIData = nullptr;
Mumble::Data *MumbleData = nullptr;
std::string AccountName;
ArcDPS::Exports ArcExports = {};

std::filesystem::path Logpath;
RotationRunType RotationRun{};
TextureMapType Globals::TextureMap{};
RenderType Globals::Render{};
std::filesystem::path Globals::SettingsPath;
float SkillIconSize = 28.0F;
std::string VersionString = std::string{VERSION_STRING};

#ifdef _DEBUG
GW2MemoryReader MemoryReader;
#endif
}; // namespace Globals
