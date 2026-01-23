#include <array>
#include <filesystem>
#include <string>

#include "arcdps/ArcDPS.h"
#include "nexus/Nexus.h"
#include "rtapi/RTAPI.hpp"
#include "mumble/Mumble.h"

#include "Render.h"
#include "Shared.h"
#include "Types.h"
#include "Version.h"

namespace Globals
{
AddonAPI_t *APIDefs = nullptr;
NexusLinkData_t *NexusLink = nullptr;
RTAPI::RealTimeData *RTAPIData = nullptr;
Mumble::Data *MumbleData = nullptr;
std::string AccountName;
ArcDPS::Exports ArcExports = {};

std::filesystem::path Logpath;
RotationLogType RotationRun{};
TextureMapType Globals::TextureMap{};
std::filesystem::path Globals::SettingsPath;
float SkillIconSize = 28.0F;
std::string VersionString = std::string{VERSION_STRING};
std::string BenchFilesLowerVersionString = std::string{LOWER_VERSION_RANGE};
std::string BenchFilesUpperVersionString = std::string{UPPER_VERSION_RANGE};

RenderDataType RenderData{};
RenderType Render{};
OptionsRenderType OptionsRender{};
RotationRenderType RotationRender{};

DownloadState BenchDataDownloadState = DownloadState::NOT_STARTED;
bool ExtractedBenchData = false;

Mumble::Identity Identity = {};

bool IsSameCast = false;
std::map<SkillID, std::chrono::steady_clock::time_point> SkillLastTimeCast = {};
std::string LastArcEventSkillName = "";

std::vector<uint32_t> CurrentlyPressedKeys = {};

SkillID LastKeyPressSkillID = SkillID{0};
std::chrono::steady_clock::time_point LastKeyPressSkillTime = std::chrono::steady_clock::now();
}; // namespace Globals
