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

// #define LOG_SKILL_FROM_BUILD
#define LOG_SKILL_IN_TIME

#ifndef _DEBUG
#ifdef LOG_SKILL_FROM_BUILD
#undef LOG_SKILL_FROM_BUILD
#endif
#ifdef LOG_SKILL_IN_TIME
#undef LOG_SKILL_IN_TIME
#endif
#endif

#define USE_ANY_SKILL_FROM_BUILD_LOGIC
#define USE_SKILL_ID_AND_NAME_MATCH_LOGIC
#define USE_TIME_FILTER_LOGIC

#ifndef USE_SKILL_ID_AND_NAME_MATCH_LOGIC
#define USE_SKILL_ID_MATCH_LOGIC
#endif

	namespace
{
	constexpr static auto MIN_TIME_DIFF = 200U;

	static std::string g_event_log_file_path;
	static bool g_event_log_initialized = false;

	void InitEventLogFile()
	{
		if (!g_event_log_initialized)
		{
			auto now = std::chrono::system_clock::now();
			auto t = std::chrono::system_clock::to_time_t(now);
			std::tm tm;
			localtime_s(&tm, &t);

			auto log_dir = std::filesystem::path{"C:/logs"};
			if (!std::filesystem::exists(log_dir))
				std::filesystem::create_directories(log_dir);

			char buf[64];
			std::strftime(buf, sizeof(buf), "eventlog_%Y%m%d_%H%M%S.csv", &tm);
			const auto log_path = log_dir / buf;
			g_event_log_file_path = log_path.string();
			std::ifstream test_file(g_event_log_file_path);
			const auto file_exists = test_file.good();
			test_file.close();
			if (!file_exists)
			{
				auto log_file = std::ofstream(g_event_log_file_path, std::ios::out);
				log_file << "Prefix,Timestamp,SrcName,SrcID,SrcProfession,SrcSpecialization,DstName,DstID,DstProfession,DstSpecialization,SkillName,SkillID\n";
				log_file.close();
			}
			g_event_log_initialized = true;
		}
	}

	void LogEvCombatDataPersistentCSV(const EvCombatDataPersistent &data, const std::string &log_prefix)
	{
		if (!g_event_log_initialized)
			InitEventLogFile();

		auto log_file = std::ofstream(g_event_log_file_path, std::ios::out | std::ios::app);
		if (log_file.is_open())
		{
			const auto now = std::chrono::system_clock::now();
			const auto t = std::chrono::system_clock::to_time_t(now);
			std::tm tm;
#ifdef _WIN32
			localtime_s(&tm, &t);
#else
			tm = *std::localtime(&t);
#endif
			char timebuf[32];
			std::strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", &tm);
			auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

			log_file << log_prefix << ','
					 << '"' << timebuf << '.' << std::setfill('0') << std::setw(3) << ms.count() << '"' << ','
					 << data.SrcName << ',' << data.SrcID << ',' << data.SrcProfession << ',' << data.SrcSpecialization << ','
					 << data.DstName << ',' << data.DstID << ',' << data.DstProfession << ',' << data.DstSpecialization << ','
					 << data.SkillName << ',' << data.SkillID << '\n';
			log_file.close();
		}
	}

	bool IsValidCombatEvent(const EvCombatData &evCbtData)
	{
		return evCbtData.src != nullptr && evCbtData.dst != nullptr && evCbtData.skillname != nullptr && evCbtData.ev != nullptr;
	}

	bool IsSkillFromBuild_IdBased(const EvCombatDataPersistent &evCbtData)
	{
		const auto &skill_info_map = rotation_run.skill_info_map;
		const auto found = skill_info_map.find(evCbtData.SkillID) != skill_info_map.end();
		return found;
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
#elif defined(USE_SKILL_ID_AND_NAME_MATCH_LOGIC)
		return IsSkillFromBuild_NameBased(evCbtData) || IsSkillFromBuild_IdBased(evCbtData);
#else
		return IsSkillFromBuild_NameBased(evCbtData);
#endif
	}

#ifdef USE_SKILL_ID_MATCH_LOGIC
	std::chrono::steady_clock::time_point UpdateCastTime(std::map<int, std::chrono::steady_clock::time_point> & last_cast_map, const EvCombatDataPersistent &evCbtData)
#else
	std::chrono::steady_clock::time_point UpdateCastTime(std::map<std::string, std::chrono::steady_clock::time_point> & last_cast_map, const EvCombatDataPersistent &evCbtData)
#endif
	{
		const auto now = std::chrono::steady_clock::now();

#ifdef USE_SKILL_ID_MATCH_LOGIC
		last_cast_map[evCbtData.SkillID] = now;
#else
		last_cast_map[evCbtData.SkillName] = now;
#endif

		return now;
	}

	std::chrono::steady_clock::time_point GetLastCastTime(const EvCombatDataPersistent &evCbtData)
	{
#ifdef USE_SKILL_ID_MATCH_LOGIC
		static auto last_cast_map = std::map<int, std::chrono::steady_clock::time_point>{};
#else
		static auto last_cast_map = std::map<std::string, std::chrono::steady_clock::time_point>{};

#endif

#ifdef USE_SKILL_ID_MATCH_LOGIC
		const auto it = last_cast_map.find(evCbtData.SkillID);
#else
		const auto it = last_cast_map.find(evCbtData.SkillName);
#endif
		if (it != last_cast_map.end())
		{
			const auto time = it->second;
			UpdateCastTime(last_cast_map, evCbtData);

			return time;
		}

		const auto now = UpdateCastTime(last_cast_map, evCbtData);
		return now;
	}

	bool IsNotTheSameCast(const EvCombatDataPersistent &evCbtData)
	{
		const auto now = std::chrono::steady_clock::now();
		const auto last_cast_time = GetLastCastTime(evCbtData);
		return (now - last_cast_time) > std::chrono::milliseconds(MIN_TIME_DIFF);
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

#ifdef USE_ANY_SKILL_FROM_BUILD_LOGIC
			if (rotation_run.skill_info_map.empty() || IsAnySkillFromBuild(data))
#endif
			{
#ifdef LOG_SKILL_FROM_BUILD
				LogEvCombatDataPersistentCSV(data, "SkillFromBuild");
#endif

#ifdef USE_TIME_FILTER_LOGIC
				if (IsNotTheSameCast(data))
#endif
				{
#ifdef LOG_SKILL_IN_TIME
					LogEvCombatDataPersistentCSV(data, "NotSameCast");
#endif

					render.skill_activation_callback(true, data);

					return true;
				}
			}
		}

		return false;
	}
}
