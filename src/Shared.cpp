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
RotationLogType RotationRun{};
TextureMapType Globals::TextureMap{};
RenderType Globals::Render{};
std::filesystem::path Globals::SettingsPath;
float SkillIconSize = 28.0F;
std::string VersionString = std::string{VERSION_STRING};
std::string BenchFilesLowerVersionString = std::string{LOWER_VERSION_RANGE};
std::string BenchFilesUpperVersionString = std::string{UPPER_VERSION_RANGE};

DownloadState BenchDataDownloadState = DownloadState::NOT_STARTED;
bool ExtractedBenchData = false;

Mumble::Identity Identity = {};

bool IsSameCast = false;
std::map<SkillID, std::chrono::steady_clock::time_point> SkillLastTimeCast = {};
std::string LastArcEventSkillName = "";
}; // namespace Globals
