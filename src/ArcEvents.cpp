///----------------------------------------------------------------------------------------------------
/// Copyright (c) Raidcore.GG - Licensed under the MIT License.
///
/// Name         :  ArcEvents.cpp
/// Description  :  Contains the callbacks for ArcDPS.
///----------------------------------------------------------------------------------------------------

#include <chrono>
#include <fstream>
#include <iomanip>
#include <map>
#include <string>

#include "ArcEvents.h"

#include "Shared.h"
#include "Types.h"

#define USE_ANY_SKILL_LOGIC
#define USE_SKILL_ID_MATCH_LOGIC

namespace
{
	std::ofstream g_event_log_file;
	bool g_event_log_initialized = false;

	void InitEventLogFile()
	{
		if (!g_event_log_initialized)
		{
			auto now = std::chrono::system_clock::now();
			auto t = std::chrono::system_clock::to_time_t(now);
			std::tm tm;
#ifdef _WIN32
			localtime_s(&tm, &t);
#else
			tm = *std::localtime(&t);
#endif
			char buf[64];
			std::strftime(buf, sizeof(buf), "eventlog_%Y%m%d_%H%M%S.csv", &tm);
			g_event_log_file.open(buf, std::ios::out | std::ios::app);
			// Write CSV header
			g_event_log_file << "Timestamp,SrcName,SrcID,SrcProfession,SrcSpecialization,DstName,DstID,DstProfession,DstSpecialization,SkillName,SkillID\n";
			g_event_log_initialized = true;
		}
	}

	void LogEvCombatDataPersistentCSV(const EvCombatDataPersistent &data)
	{
		if (!g_event_log_initialized)
			InitEventLogFile();

		if (g_event_log_file.is_open())
		{
			auto now = std::chrono::system_clock::now();
			auto t = std::chrono::system_clock::to_time_t(now);
			std::tm tm;
#ifdef _WIN32
			localtime_s(&tm, &t);
#else
			tm = *std::localtime(&t);
#endif
			char timebuf[32];
			std::strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", &tm);
			auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

			g_event_log_file << '"' << timebuf << '.' << std::setfill('0') << std::setw(3) << ms.count() << '"' << ','
							 << data.SrcName << ',' << data.SrcID << ',' << data.SrcProfession << ',' << data.SrcSpecialization << ','
							 << data.DstName << ',' << data.DstID << ',' << data.DstProfession << ',' << data.DstSpecialization << ','
							 << data.SkillName << ',' << data.SkillID << '\n';
			g_event_log_file.flush();
		}
	}

	bool IsValidCombatEvent(const EvCombatData &evCbtData)
	{
		return evCbtData.src != nullptr && evCbtData.dst != nullptr && evCbtData.skillname != nullptr && evCbtData.ev != nullptr;
	}

	bool IsSkillFromBuild_IdBased(const EvCombatDataPersistent &evCbtData)
	{
		const auto &skill_info_map = rotation_run.skill_info_map;
		return skill_info_map.find(evCbtData.SkillID) != skill_info_map.end();
	}

	bool IsSkillFromBuild_NameBased(const EvCombatDataPersistent &evCbtData)
	{
		const auto &skill_info_map = rotation_run.skill_info_map;
		for (const auto &kv : skill_info_map)
		{
			if (kv.second.name == evCbtData.SkillName)
				return true;
		}
		return false;
	}

	bool IsAnySkillFromBuild(const EvCombatDataPersistent &evCbtData)
	{
#ifdef USE_SKILL_ID_MATCH_LOGIC
		return IsSkillFromBuild_IdBased(evCbtData);
#else
		return IsSkillFromBuild_NameBased(evCbtData);
#endif
	}
};

namespace ArcEv
{
	void OnCombatLocal(
		ArcDPS::CombatEvent *ev,
		ArcDPS::AgentShort *src,
		ArcDPS::AgentShort *dst,
		char *skillname,
		uint64_t id,
		uint64_t revision)
	{
		OnCombat("EV_ARCDPS_COMBATEVENT_LOCAL_RAW", ev, src, dst, skillname, id, revision);
	}

	bool OnCombat(
		const char *channel,
		ArcDPS::CombatEvent *ev,
		ArcDPS::AgentShort *src,
		ArcDPS::AgentShort *dst,
		char *skillname,
		uint64_t id, uint64_t revision)
	{
#ifdef GW2_NEXUS_ADDON
		if (APIDefs == nullptr)
			return false;
#endif

		auto evCbtData = EvCombatData{
			ev,
			src,
			dst,
			skillname,
			id,
			revision};

#ifdef GW2_NEXUS_ADDON
		APIDefs->Events.Raise(channel, &evCbtData);
#endif

		if (IsValidCombatEvent(evCbtData))
		{
			const auto data = EvCombatDataPersistent{
				.SrcName = std::string(evCbtData.src->Name),
				.SrcID = evCbtData.src->ID,
				.SrcProfession = evCbtData.src->Profession,
				.SrcSpecialization = evCbtData.src->Specialization,
				.DstName = std::string(evCbtData.dst->Name),
				.DstID = evCbtData.dst->ID,
				.DstProfession = evCbtData.dst->Profession,
				.DstSpecialization = evCbtData.dst->Specialization,
				.SkillName = std::string(evCbtData.skillname),
				.SkillID = evCbtData.id};

#ifdef _DEBUG
			LogEvCombatDataPersistentCSV(data);
#endif

#ifdef USE_ANY_SKILL_LOGIC
			if (IsAnySkillFromBuild(data))
#endif
			{
				render.key_press_cb(true, data);

				return true;
			}
		}

		return false;
	}
}
